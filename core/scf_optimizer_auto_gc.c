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

#define AUTO_GC_BB_SPLIT(parent, child0, child1) \
	do { \
		int ret = scf_basic_block_split(parent, &child0); \
		if (ret < 0) \
			return ret; \
		\
		ret = scf_basic_block_split(child0, &child1); \
		if (ret < 0) \
			return ret; \
		\
		child0->call_flag        = 1; \
		child1->call_flag        = parent->call_flag; \
		child1->dereference_flag = parent->dereference_flag; \
		\
		SCF_XCHG(parent->dn_malloced, child1->dn_malloced); \
		\
		scf_list_add_front(&parent->list, &child0->list); \
		scf_list_add_front(&child0->list, &child1->list); \
	} while (0)

static int _optimize_auto_gc_bb(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;
	scf_3ac_code_t* c_free;
	scf_3ac_code_t* c_ref;

	scf_basic_block_t* bb_prev = NULL;
	scf_basic_block_t* cur_bb  = bb;

	int i;
	int j;
	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < bb_prev->dn_malloced->size; j++) {

			scf_dag_node_t* dn = bb_prev->dn_malloced->data[j];

			assert(0 == scf_vector_add_unique(bb->dn_malloced, dn));
		}
	}

	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < bb->dn_malloced->size; j++) {

			scf_dag_node_t* dn = bb->dn_malloced->data[j];
			scf_variable_t* v0 = dn->var;

			if (!scf_vector_find(bb_prev->dn_malloced, dn)) {
				scf_loge("auto gc error: bb: %p, not all v_%d_%d/%s in previous blocks malloced automaticly\n",
						bb, v0->w->line, v0->w->pos, v0->w->text->data);
				return -1;
			}
		}
	}

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		if (SCF_OP_ASSIGN != c->op->type)
			goto _end;

		scf_variable_t* v0 = c->dst->dag_node->var;

		if (!scf_variable_is_struct_pointer(v0))
			goto _end;

		if (scf_vector_find(cur_bb->dn_malloced, c->dst->dag_node)) {

			scf_basic_block_t* bb1     = NULL;
			scf_basic_block_t* bb2     = NULL;

			AUTO_GC_BB_SPLIT(cur_bb, bb1, bb2);

			c_free = _auto_gc_code_alloc("scf_free", &c->dst, 1);
			c_free->basic_block = bb1;

			scf_list_add_tail(&bb1->code_list_head, &c_free->list);

			assert(0 == scf_vector_add_unique(bb1->dn_freed, c->dst->dag_node));

			assert(0 == scf_vector_del(bb2->dn_malloced, c->dst->dag_node));

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

				assert(0 == scf_vector_add_unique(cur_bb->dn_malloced, c->dst->dag_node));

				scf_vector_del(cur_bb->dn_freed, c->dst->dag_node);
			}
		} else {

			if (scf_vector_find(cur_bb->dn_malloced, dn)) {

				assert(!scf_vector_find(cur_bb->dn_freed, dn));

				auto_gc_flag = 1;

				scf_basic_block_t* bb3 = NULL;
				scf_basic_block_t* bb4 = NULL;

				AUTO_GC_BB_SPLIT(cur_bb, bb3, bb4);

				c_ref = _auto_gc_code_alloc("scf_ref", &c->dst, 1);
				c_ref->basic_block = bb3;

				scf_list_add_tail(&bb3->code_list_head, &c_ref->list);

				assert(0 == scf_vector_add_unique(bb4->dn_malloced, c->dst->dag_node));

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				scf_loge("v0_%d_%d/%s, auto_gc_flag: %d->%d\n", v0->w->line, v0->w->pos, v0->w->text->data,
						v0->auto_gc_flag, auto_gc_flag);

				v0->auto_gc_flag = auto_gc_flag;

				cur_bb = bb4;
				continue;
			}
		}

		scf_loge("v0_%d_%d/%s, auto_gc_flag: %d->%d\n", v0->w->line, v0->w->pos, v0->w->text->data,
				v0->auto_gc_flag, auto_gc_flag);

		v0->auto_gc_flag = auto_gc_flag;

_end:
		if (cur_bb != bb) {
			scf_list_del(&c->list);
			scf_list_add_tail(&cur_bb->code_list_head, &c->list);
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

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		ret = _optimize_auto_gc_bb(bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	cur_bb = bb;
	int j;
	for (j = 0; j < bb->dn_malloced->size; j++) {

		scf_dag_node_t*    dn = bb->dn_malloced->data[j];
		scf_variable_t*    v0 = dn->var;

		scf_3ac_code_t*    c_free;

		scf_basic_block_t* bb1 = NULL;

		int ret = scf_basic_block_split(cur_bb, &bb1);
		if (ret < 0)
			return ret;

		bb1->call_flag = 1;

		c_free = _auto_gc_code_alloc_dn("scf_free", dn);
		c_free->basic_block = bb1;

		scf_list_add_tail(&bb1->code_list_head, &c_free->list);

		scf_list_add_front(&cur_bb->list, &bb1->list);

		scf_loge("last bb: %p, need free: v_%d_%d/%s\n", bb, v0->w->line, v0->w->pos, v0->w->text->data);

		cur_bb = bb1;
	}

	if (cur_bb != bb) {
		cur_bb->end_flag = 1;

		bb->call_flag  = 1;
		bb->end_flag   = 0;

		scf_list_t* h0 = &bb->code_list_head;
		scf_list_t* h1 = &cur_bb->code_list_head;
		scf_list_t  h;

		scf_list_init(&h);

		scf_list_mov(&h, h0, scf_3ac_code_t, list);
		scf_list_mov(h0, h1, scf_3ac_code_t, list);
		scf_list_mov(h1, &h, scf_3ac_code_t, list);
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_auto_gc =
{
	.name     =  "auto_gc",

	.optimize =  _optimize_auto_gc,
};

