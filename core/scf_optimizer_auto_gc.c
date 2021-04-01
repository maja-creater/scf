#include"scf_optimizer.h"

static scf_3ac_code_t* _auto_gc_code_alloc(const char* fname, scf_3ac_operand_t** operands, int nb_operands)
{
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();

	int i;
	for (i = 0; i < nb_operands; i++) {

		scf_3ac_operand_t* src  = scf_3ac_operand_alloc();

		scf_vector_add(srcs, src);

		src->node     = operands[i]->node;
		src->dag_node = operands[i]->dag_node;
	}

	c->op   = scf_3ac_find_operator(SCF_OP_3AC_CALL_EXTERN);
	c->dst	= NULL;
	c->srcs = srcs;
	c->extern_fname = scf_string_cstr(fname);

	return c;
}

static scf_3ac_code_t* _auto_gc_code_alloc_dn(const char* fname, scf_dag_node_t* dn)
{
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();

	scf_3ac_operand_t* src  = scf_3ac_operand_alloc();

	scf_vector_add(srcs, src);

	src->node     = dn->node;
	src->dag_node = dn;

	c->op   = scf_3ac_find_operator(SCF_OP_3AC_CALL_EXTERN);
	c->dst	= NULL;
	c->srcs = srcs;
	c->extern_fname = scf_string_cstr(fname);

	return c;
}

#define AUTO_GC_BB_SPLIT(parent, child) \
	do { \
		int ret = scf_basic_block_split(parent, &child); \
		if (ret < 0) \
			return ret; \
		\
		child->call_flag        = parent->call_flag; \
		child->dereference_flag = parent->dereference_flag; \
		\
		SCF_XCHG(parent->dn_malloced, child->dn_malloced); \
		\
		scf_list_add_front(&parent->list, &child->list); \
	} while (0)

static int _optimize_auto_gc_bb(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;
	scf_3ac_code_t* c_free;
	scf_3ac_code_t* c_ref;
	scf_vector_t*   dn_malloced;

	scf_basic_block_t* bb_prev = NULL;
	scf_basic_block_t* cur_bb  = bb;

	dn_malloced = scf_vector_alloc();
	if (!dn_malloced)
		return -ENOMEM;

	// at first, the malloced vars, are ones malloced in previous blocks
	int i;
	int j;
	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < bb_prev->dn_malloced->size; j++) {

			scf_dag_node_t* dn = bb_prev->dn_malloced->data[j];

			if (scf_vector_find(bb_prev->dn_freed, dn))
				continue;

			assert(0 == scf_vector_add_unique(dn_malloced, dn));
		}
	}

	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < dn_malloced->size; j++) {

			scf_dag_node_t* dn = dn_malloced->data[j];
			scf_variable_t* v0 = dn->var;

			if (!scf_vector_find(bb_prev->dn_malloced, dn)
					|| scf_vector_find(bb_prev->dn_freed, dn)) {

				scf_loge("auto gc error: bb: %p, not all v_%d_%d/%s in previous blocks malloced automaticly\n",
						bb, v0->w->line, v0->w->pos, v0->w->text->data);
				return -1;
			}
		}
	}
#if 1
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		if (SCF_OP_ASSIGN != c->op->type)
			goto _end;

		scf_variable_t* v0 = c->dst->dag_node->var;

		if (!scf_variable_is_struct_pointer(v0))
			goto _end;

		if (scf_vector_find(dn_malloced, c->dst->dag_node)) {

			scf_basic_block_t* bb1     = NULL;
			scf_basic_block_t* bb2     = NULL;

			AUTO_GC_BB_SPLIT(cur_bb, bb1);
			AUTO_GC_BB_SPLIT(bb1,    bb2);

			bb1->call_flag = 1;
			bb1->dereference_flag = 0;

			c_free = _auto_gc_code_alloc("scf_free", &c->dst, 1);
			c_free->basic_block = bb1;

			scf_list_add_tail(&bb1->code_list_head, &c_free->list);

			assert(0 == scf_vector_del(dn_malloced, c->dst->dag_node));

			cur_bb = bb2;
		}

		scf_3ac_operand_t* src  = c->srcs->data[0];
		scf_dag_node_t*    dn   = src->dag_node;

		while (dn) {
			if (SCF_OP_TYPE_CAST == dn->type)
				dn = dn->childs->data[0];

			else if (SCF_OP_EXPR == dn->type)
				dn = dn->childs->data[0];
			else
				break;
		}

		int auto_gc_flag = 0;

		if (SCF_OP_CALL == dn->type) {

			scf_dag_node_t* dn_pf = dn->childs->data[0];
			scf_function_t* f     = dn_pf->var->func_ptr;

			if (!strcmp(f->node.w->text->data, "scf_malloc")) {
				auto_gc_flag = 1;

				assert(0 == scf_vector_add_unique(dn_malloced, c->dst->dag_node));
			}
		} else {
			if (scf_vector_find(dn_malloced, dn)) {

				auto_gc_flag = 1;

				scf_basic_block_t* bb3 = NULL;
				scf_basic_block_t* bb4 = NULL;

				AUTO_GC_BB_SPLIT(cur_bb, bb3);

				c_ref = _auto_gc_code_alloc("scf_ref", &c->dst, 1);
				c_ref->basic_block = bb3;

				scf_list_add_tail(&bb3->code_list_head, &c_ref->list);

				assert(0 == scf_vector_add_unique(dn_malloced, c->dst->dag_node));

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				scf_loge("v0_%d_%d/%s, auto_gc_flag: %d\n", v0->w->line, v0->w->pos, v0->w->text->data,
						auto_gc_flag);

				if (l != scf_list_sentinel(&bb->code_list_head)) {

					AUTO_GC_BB_SPLIT(bb3, bb4);
					cur_bb = bb4;
				} else
					cur_bb = bb3;

				bb3->call_flag = 1;
				bb3->dereference_flag = 0;
				continue;
			}
		}

		scf_loge("v0_%d_%d/%s, auto_gc_flag: %d\n", v0->w->line, v0->w->pos, v0->w->text->data,
				auto_gc_flag);

_end:
		if (cur_bb != bb) {
			scf_list_del(&c->list);
			scf_list_add_tail(&cur_bb->code_list_head, &c->list);
		}
	}
#endif
	return 0;
}

static int _auto_gc_bb_find(scf_basic_block_t* bb)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	int count = 0;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		if (SCF_OP_ASSIGN != c->op->type)
			continue;

		scf_variable_t* v0 = c->dst->dag_node->var;

		if (!scf_variable_is_struct_pointer(v0))
			continue;

		assert(0 == scf_vector_add_unique(bb->dn_freed, c->dst->dag_node));

		scf_vector_del(bb->dn_malloced, c->dst->dag_node);
		count++;

		scf_3ac_operand_t* src  = c->srcs->data[0];
		scf_dag_node_t*    dn   = src->dag_node;

		while (dn) {
			if (SCF_OP_TYPE_CAST == dn->type)
				dn = dn->childs->data[0];

			else if (SCF_OP_EXPR == dn->type)
				dn = dn->childs->data[0];
			else
				break;
		}

		int auto_gc_flag = 0;

		if (SCF_OP_CALL == dn->type) {

			scf_dag_node_t* dn_pf = dn->childs->data[0];
			scf_function_t* f     = dn_pf->var->func_ptr;

			if (!strcmp(f->node.w->text->data, "scf_malloc")) {
				auto_gc_flag = 1;

				assert(0 == scf_vector_add_unique(bb->dn_malloced, c->dst->dag_node));

				scf_vector_del(bb->dn_freed, c->dst->dag_node);

				count++;
			}
		} else {
			if (scf_vector_find(bb->dn_malloced, dn)
					&& !scf_vector_find(bb->dn_freed, dn)) {

				auto_gc_flag = 1;

				assert(0 == scf_vector_add_unique(bb->dn_malloced, c->dst->dag_node));

				scf_vector_del(bb->dn_freed, c->dst->dag_node);

				count++;
			}
		}

		scf_loge("bb: %p, v0_%d_%d/%s, auto_gc_flag: %d\n", bb,
				v0->w->line, v0->w->pos, v0->w->text->data,
				auto_gc_flag);
	}

	return count;
}

static int _auto_gc_bb_next_find(scf_basic_block_t* bb, void* data, scf_vector_t* queue)
{
	scf_basic_block_t* next_bb;
	scf_dag_node_t*    dn;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		next_bb   = bb->nexts->data[j];

		int k;
		for (k = 0; k < bb->dn_malloced->size; k++) {

			dn =        bb->dn_malloced->data[k];

			if (scf_vector_find(bb->dn_freed, dn))
				continue;

			if (scf_vector_find(next_bb->dn_malloced, dn))
				continue;

			if (scf_vector_find(next_bb->dn_freed, dn))
				continue;

			ret = scf_vector_add(next_bb->dn_malloced, dn);
			if (ret < 0)
				return ret;
			++count;
		}

		ret = scf_vector_add(queue, next_bb);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _find_dn_malloced(scf_basic_block_t* bb, void* data)
{
	if (!scf_vector_find(bb->dn_malloced, data))
		return 0;

	if (scf_vector_find(bb->dn_freed, data)) {
		scf_loge("error free dn: \n");
		return -1;
	}

	if (scf_vector_find(bb->dn_updateds, data))
		return 1;
	return 0;
}

static int _find_dn_active(scf_basic_block_t* bb, void* data)
{
	if (scf_vector_find(bb->dn_loads, data))
		return 1;

	if (scf_vector_find(bb->dn_reloads, data))
		return 1;
	return 0;
}

static int _bb_find_dn_malloced(scf_basic_block_t* root, scf_list_t* bb_list_head, scf_dag_node_t* dn, scf_vector_t* results)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb->visited_flag = 0;
	}

	return scf_basic_block_search_dfs_prev(root, _find_dn_malloced, dn, results);
}

static int _bb_find_dn_active(scf_basic_block_t* root, scf_list_t* bb_list_head, scf_dag_node_t* dn, scf_vector_t* results)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb->visited_flag = 0;
	}

	return scf_basic_block_search_dfs_prev(root, _find_dn_active, dn, results);
}

static int _bb_next_del(scf_basic_block_t* bb, void* data, scf_vector_t* queue)
{
	scf_basic_block_t* next_bb;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		next_bb   = bb->nexts->data[j];

		scf_vector_del(next_bb->dn_malloced, data);

		++count;

		ret = scf_vector_add(queue, next_bb);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _bb_add_last_free(scf_function_t* f, scf_basic_block_t* bb, scf_dag_node_t* dn)
{
	scf_basic_block_t* bb1    = NULL;
	scf_3ac_code_t*    c_free = _auto_gc_code_alloc_dn("scf_free", dn);
	scf_dag_node_t*    dn2;

	int ret = scf_basic_block_split(bb, &bb1);
	if (ret < 0)
		return ret;

	bb1->call_flag = 1;
	bb1->end_flag  = bb->end_flag;

	c_free->basic_block = bb1;

	scf_list_add_tail(&bb1->code_list_head, &c_free->list);

	scf_list_add_front(&bb->list, &bb1->list);

	scf_vector_del(bb->dn_malloced, dn);

	int i;
	for (i  = 0; i < bb->dn_malloced->size; i++) {
		dn2 =        bb->dn_malloced->data[i];

		if (scf_vector_find(bb->dn_freed, dn2))
			continue;

		ret = scf_vector_add(bb1->dn_malloced, dn2);
		if (ret < 0)
			return ret;
	}

	return scf_basic_block_search_bfs(bb1, _bb_next_del, dn);
}

static int _auto_gc_last_free(scf_function_t* f, scf_list_t* bb_list_head)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	l  = scf_list_tail(bb_list_head);
	bb = scf_list_data(l, scf_basic_block_t, list);

	while (bb->dn_malloced->size > 0) {

		scf_dag_node_t* dn = bb->dn_malloced->data[0];
		scf_variable_t* v  = dn->var;

		scf_loge("last free: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		scf_vector_t* vec;

		vec = scf_vector_alloc();
		if (!vec)
			return -ENOMEM;

		scf_basic_block_t* bb1;
		scf_basic_block_t* bb2;
		scf_basic_block_t* bb3;
		scf_basic_block_t* bb_dominator;

		int dfo = 0;
		int j;

		int ret = _bb_find_dn_malloced(bb, bb_list_head, dn, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}

#define AUTO_GC_FIND_MAX_DFO() \
		do { \
			for (j  = 0; j < vec->size; j++) { \
				bb1 =        vec->data[j]; \
				\
				if (bb1->depth_first_order > dfo) \
					dfo = bb1->depth_first_order; \
			} \
		} while (0)

		AUTO_GC_FIND_MAX_DFO();

		vec->size = 0;

		ret = _bb_find_dn_active(bb, bb_list_head, dn, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}
		AUTO_GC_FIND_MAX_DFO();

		for (j = 0; j    < bb->dominators->size; j++) {
			bb_dominator = bb->dominators->data[j];

			if (bb_dominator->depth_first_order >= dfo)
				break;
		}

		vec->size = 0;

		for (j  = 0; j < bb_dominator->prevs->size; j++) {
			bb1 =        bb_dominator->prevs->data[j];

			if (!scf_vector_find(bb1->dn_malloced, dn))
				continue;

			if (scf_vector_find(bb1->dn_freed, dn))
				continue;

			ret = scf_vector_add(vec, bb1);
			if (ret < 0) {
				scf_vector_free(vec);
				return ret;
			}
		}

		assert(vec->size >= 1);

		if (vec->size == bb_dominator->prevs->size) {

			ret = _bb_add_last_free(f, bb_dominator, dn);

			scf_vector_free(vec);
			vec = NULL;

			if (ret < 0)
				return ret;

		} else if (1 == vec->size) {

			ret = _bb_add_last_free(f, vec->data[0], dn);

			scf_vector_free(vec);
			vec = NULL;

			if (ret < 0)
				return ret;

		} else {
			bb1 = scf_basic_block_alloc();
			if (!bb1) {
				scf_vector_free(vec);
				return -ENOMEM;
			}

			bb1->call_flag = 1;

			scf_vector_add(bb1->nexts, bb_dominator);

			scf_vector_free(bb1->prevs);
			bb1->prevs = vec;
			vec = NULL;

			scf_list_add_tail(&bb_dominator->list, &bb1->list);

			for (j  = 0; j < bb1->prevs->size; j++) {
				bb2 =        bb1->prevs->data[j];

				assert(0 == scf_vector_del(bb_dominator->prevs, bb2));

				int k;
				for (k = 0; k < bb2->nexts->size; k++) {

					if (bb2->nexts->data[k] == bb_dominator)
						bb2->nexts->data[k] =  bb1;
				}
			}
			scf_vector_add(bb_dominator->prevs, bb1);

			for (j  = 0; j < bb1->prevs->size; j++) {
				bb2 =        bb1->prevs->data[j];

				for (l  = &bb2->list; l != &bb1->list; l = scf_list_next(l)) {

					bb3 = scf_list_data(l, scf_basic_block_t, list);

					if (!bb3->jmp_flag)
						continue;

					if (scf_vector_find(bb_dominator->prevs, bb3))
						continue;

					scf_list_t*     l2;
					scf_3ac_code_t* c;

					for (l2 = scf_list_head(&bb3->code_list_head); l2 != scf_list_sentinel(&bb3->code_list_head); ) {

						c   = scf_list_data(l2, scf_3ac_code_t, list);
						l2  = scf_list_next(l2);

						if (c->dst->bb == bb_dominator)
							c->dst->bb = bb1;
					}
				}

				scf_dag_node_t* dn2;
				int k;
				for (k  = 0; k < bb2->dn_malloced->size; k++) {
					dn2 =        bb2->dn_malloced->data[k];

					if (dn2 == dn || scf_vector_find(bb2->dn_freed, dn2))
						continue;

					ret = scf_vector_add(bb1->dn_malloced, dn2);
					if (ret < 0)
						return ret;
				}
			}

			scf_3ac_code_t* c_free = _auto_gc_code_alloc_dn("scf_free", dn);

			c_free->basic_block = bb1;

			scf_list_add_tail(&bb1->code_list_head, &c_free->list);

			ret = scf_basic_block_search_bfs(bb1, _bb_next_del, dn);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int _optimize_auto_gc(scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_basic_block_t* cur_bb;

	int count;
	int ret;
	int i;

	int n = 0;
	do {
		scf_loge("n: %d\n", n);
		for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

			bb = scf_list_data(l, scf_basic_block_t, list);
			l  = scf_list_next(l);

			ret = _auto_gc_bb_find(bb);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		printf("\n");

		l   = scf_list_head(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);

		ret = scf_basic_block_search_bfs(bb, _auto_gc_bb_next_find, NULL);
		if (ret < 0)
			return ret;
		count = ret;
		n++;
	} while (count > 0);

	ret = _auto_gc_last_free(f, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		ret = _optimize_auto_gc_bb(bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_auto_gc =
{
	.name     =  "auto_gc",

	.optimize =  _optimize_auto_gc,
};

