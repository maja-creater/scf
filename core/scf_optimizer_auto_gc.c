#include"scf_optimizer.h"

static scf_3ac_code_t* _auto_gc_code_alloc_dn(scf_ast_t* ast, const char* fname, scf_dag_node_t* dn)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_function_t* f;
	scf_variable_t* v;
	scf_type_t*     t;
	scf_dag_node_t* dn_pf;

	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();

	s = scf_string_cstr("global");
	w = scf_lex_word_alloc(s, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr(fname);

	f = scf_ast_find_function(ast, fname);
	assert(f);

	t = scf_ast_find_type_type(ast, SCF_FUNCTION_PTR);
	v = SCF_VAR_ALLOC_BY_TYPE(f->node.w, t, 1, 1, f);
	assert(v);
	v->const_literal_flag = 1;

	dn_pf = scf_dag_node_alloc(v->type, v, (scf_node_t*)f);
	assert(dn_pf);

	scf_3ac_operand_t* src0  = scf_3ac_operand_alloc();
	scf_3ac_operand_t* src1  = scf_3ac_operand_alloc();

	scf_vector_add(srcs, src0);
	scf_vector_add(srcs, src1);

	src0->node     = (scf_node_t*)f; 
	src0->dag_node = dn_pf;

	src1->node     = dn->node;
	src1->dag_node = dn;

	c->op   = scf_3ac_find_operator(SCF_OP_CALL);
	c->dsts = NULL;
	c->srcs = srcs;

	return c;
}

static scf_3ac_code_t* _code_alloc_dereference(scf_ast_t* ast, scf_dag_node_t* dn)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   src0  = scf_3ac_operand_alloc();
	scf_vector_add(srcs, src0);

	src0->node     = dn->node;
	src0->dag_node = dn;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_DEREFERENCE);

	w = scf_lex_word_alloc(dn->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("*");

	scf_type_t*     t   = scf_ast_find_type_type(ast, dn->var->type);
	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn->var->nb_pointers - 1, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dst0->dag_node = dn2;

	return c;
}

static int _auto_gc_code_list_for_ds(scf_list_t* h, scf_ast_t* ast, const char* fname, scf_dn_status_t* ds)
{
	scf_dag_node_t*    dn = ds->dag_node;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* dst;

	if (ds->dn_indexes) {

		int i;
		for (i = ds->dn_indexes->size - 1; i >= 0; i--) {

			scf_dn_index_t* di = ds->dn_indexes->data[i];

			assert(!di->member);
			assert(di->index >= 0);

			assert(0 == di->index);

			c = _code_alloc_dereference(ast, dn);

			scf_list_add_tail(h, &c->list);

			dst = c->dsts->data[0];
			dn  = dst->dag_node;
		}
	}

	c = _auto_gc_code_alloc_dn(ast, fname, dn);

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int __ds_for_dn(scf_dn_status_t* ds, scf_dag_node_t* dn_base)
{
	scf_dag_node_t*   dn_index;
	scf_dn_index_t*   di;

	int ret;

	while (SCF_OP_DEREFERENCE == dn_base->type) {

		di = scf_dn_index_alloc();
		if (!di)
			return -ENOMEM;
		di->index = 0;

		ret = scf_vector_add(ds->dn_indexes, di);
		if (ret < 0) {
			scf_dn_index_free(di);
			return -ENOMEM;
		}

		dn_base  = dn_base->childs->data[0];
	}

	while (SCF_OP_ARRAY_INDEX == dn_base->type
			|| SCF_OP_POINTER == dn_base->type) {

		dn_index = dn_base->childs->data[1];

		ret = scf_dn_status_index(ds, dn_index, dn_base->type);
		if (ret < 0)
			return ret;

		dn_base  = dn_base->childs->data[0];
	}
	assert(scf_type_is_var(dn_base->type));

	ds->dag_node = dn_base;
	return 0;
}

static int _ds_for_dn(scf_dn_status_t** pds, scf_dag_node_t* dn)
{
	scf_dn_status_t*  ds;

	ds = calloc(1, sizeof(scf_dn_status_t));
	if (!ds)
		return -ENOMEM;

	ds->dn_indexes = scf_vector_alloc();
	if (!ds->dn_indexes) {
		free(ds);
		return -ENOMEM;
	}

	int ret = __ds_for_dn(ds, dn);
	if (ret < 0) {
		scf_dn_status_free(ds);
		return ret;
	}

	if (0 == ds->dn_indexes->size) {
		scf_vector_free(ds->dn_indexes);
		ds->dn_indexes = NULL;
	}

	*pds = ds;
	return 0;
}

static int _ds_for_assign_dereference(scf_dn_status_t** pds, scf_dag_node_t* dn)
{
	scf_dn_status_t*  ds;
	scf_dag_node_t*   dn_index;
	scf_dn_index_t*   di;

	ds = calloc(1, sizeof(scf_dn_status_t));
	if (!ds)
		return -ENOMEM;

	ds->dn_indexes = scf_vector_alloc();
	if (!ds->dn_indexes) {
		free(ds);
		return -ENOMEM;
	}

	di = scf_dn_index_alloc();
	if (!di) {
		scf_dn_status_free(ds);
		return -ENOMEM;
	}
	di->index = 0;

	int ret = scf_vector_add(ds->dn_indexes, di);
	if (ret < 0) {
		scf_dn_index_free(di);
		scf_dn_status_free(ds);
		return ret;
	}

	ret = __ds_for_dn(ds, dn);
	if (ret < 0) {
		scf_dn_status_free(ds);
		return ret;
	}

	*pds = ds;
	return 0;
}

static int _find_ds_malloced(scf_basic_block_t* bb, void* data)
{
	scf_dn_status_t* ds = data;

	if (!scf_vector_find_cmp(bb->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes))
		return 0;

	if (scf_vector_find_cmp(bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes)) {
		scf_loge("error free dn: \n");
		return -1;
	}

	if (scf_vector_find(bb->dn_updateds, ds->dag_node))
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

static int _bb_find_ds_malloced(scf_basic_block_t* root, scf_list_t* bb_list_head, scf_dn_status_t* ds, scf_vector_t* results)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb->visited_flag = 0;
	}

	return scf_basic_block_search_dfs_prev(root, _find_ds_malloced, ds, results);
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

static int _bb_prev_add_active(scf_basic_block_t* bb, void* data, scf_vector_t* queue)
{
	scf_basic_block_t* bb_prev;

	scf_dag_node_t* dn = data;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->prevs->size; j++) {
		bb_prev   = bb->prevs->data[j];

		if (!scf_vector_find(bb_prev->exit_dn_aliases, dn)) {

			ret = scf_vector_add_unique(bb_prev->exit_dn_actives, dn);
			if (ret < 0)
				return ret;
		}

		if (scf_vector_find(bb_prev->dn_updateds, dn)) {

			if (scf_vector_find(bb_prev->exit_dn_aliases, dn)
					|| scf_type_is_operator(dn->type))

				ret = scf_vector_add_unique(bb_prev->dn_resaves, dn);
			else
				ret = scf_vector_add_unique(bb_prev->dn_saves, dn);

			if (ret < 0)
				return ret;
		}

		++count;

		ret = scf_vector_add(queue, bb_prev);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _bb_add_active(scf_basic_block_t* bb, scf_dag_node_t* dn)
{
	int ret = scf_vector_add_unique(bb->entry_dn_actives, dn);
	if (ret < 0)
		return ret;

	if (scf_type_is_operator(dn->type))
		ret = scf_vector_add(bb->dn_reloads, dn);
	else
		ret = scf_vector_add(bb->dn_loads, dn);

	return ret;
}

static int _bb_add_gc_code(scf_ast_t* ast, scf_basic_block_t* bb, scf_dn_status_t* ds, const char* fname)
{
	scf_3ac_code_t* c;
	scf_list_t*     l;
	scf_list_t      h;
	scf_list_init(&h);

	if (scf_vector_add_unique(bb->dn_reloads, ds->dag_node) < 0)
		return -ENOMEM;

	int ret = _auto_gc_code_list_for_ds(&h, ast, fname, ds);
	if (ret < 0)
		return ret;

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}
	return 0;
}

static int _bb_add_free(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_dn_status_t* ds)
{
	scf_basic_block_t* bb1     = NULL;
	scf_dn_status_t*   ds2     = NULL;
	scf_dag_node_t*    dn      = ds->dag_node;
	scf_dag_node_t*    dn2;

	int ret = scf_basic_block_split(bb, &bb1);
	if (ret < 0)
		return ret;

	scf_list_add_front(&bb->list, &bb1->list);

	ds2 = scf_vector_find_cmp(bb->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes);

	assert(0 == scf_vector_del(bb->ds_malloced, ds2));

	if (ds2 != ds)
		scf_dn_status_free(ds2);
	ds2 = NULL;

	int i;
	for (i  = 0; i < bb->ds_malloced->size; i++) {
		ds2 =        bb->ds_malloced->data[i];

		if (scf_vector_find_cmp(bb->ds_freed, ds2, scf_dn_status_cmp_same_dn_indexes))
			continue;

		scf_dn_status_t* ds3 = scf_dn_status_clone(ds2);

		ret = scf_vector_add(bb1->ds_malloced, ds2);
		if (ret < 0)
			return ret;
	}

	if (bb->end_flag) {

		scf_basic_block_mov_code(scf_list_head(&bb->code_list_head), bb1, bb);

		bb1->end_flag  = 1;
		bb ->end_flag  = 0;
		bb ->call_flag = 1;

		bb1 = bb;
	} else {
		bb1->call_flag = 1;
	}

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	bb1->auto_free_flag = 1;

	ret = _bb_add_gc_code(ast, bb1, ds, "scf_free");
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	return scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
}

static int _bb_split_prev_add_free(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_list_t* bb_list_head, scf_dn_status_t* ds, scf_vector_t* bb_split_prevs)
{
	scf_basic_block_t* bb1;
	scf_basic_block_t* bb2;
	scf_basic_block_t* bb3;
	scf_dag_node_t*    dn   = ds->dag_node;

	bb1 = scf_basic_block_alloc();
	if (!bb1) {
		scf_vector_free(bb_split_prevs);
		return -ENOMEM;
	}

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	bb1->call_flag      = 1;
	bb1->auto_free_flag = 1;

	scf_vector_add( bb1->nexts, bb);
	scf_vector_free(bb1->prevs);

	bb1->prevs     = bb_split_prevs;
	bb_split_prevs = NULL;

	scf_list_add_tail(&bb->list, &bb1->list);

	int j;
	for (j  = 0; j < bb1->prevs->size; j++) {
		bb2 =        bb1->prevs->data[j];

		assert(0 == scf_vector_del(bb->prevs, bb2));

		int k;
		for (k = 0; k < bb2->nexts->size; k++) {

			if (bb2->nexts->data[k] == bb)
				bb2->nexts->data[k] =  bb1;
		}
	}

	for (j  = 0; j < bb->prevs->size; j++) {
		bb2 =        bb->prevs->data[j];

		scf_list_t*        l;
		scf_3ac_code_t*    c;
		scf_3ac_operand_t* dst;

		for (l  = scf_list_next(&bb2->list); l != scf_list_sentinel(bb_list_head);
			 l  = scf_list_next(l)) {

			bb3 = scf_list_data(l, scf_basic_block_t, list);

			if (bb3->jcc_flag)
				continue;

			if (bb3->jmp_flag)
				break;

			if (bb3 == bb1) {
				bb3 = scf_basic_block_alloc();
				if (!bb3)
					return -ENOMEM;
				bb3->jmp_flag = 1;

				c       = scf_3ac_code_alloc();
				c->op   = scf_3ac_find_operator(SCF_OP_GOTO);
				c->dsts = scf_vector_alloc();

				dst     = scf_3ac_operand_alloc();
				dst->bb = bb;

				if (scf_vector_add(c->dsts, dst) < 0)
					return -ENOMEM;

				c->basic_block = bb3;

				assert(0 == scf_vector_add(f->jmps, c));

				scf_list_add_tail(&bb3->code_list_head, &c->list);

				scf_list_add_tail(&bb1->list, &bb3->list);
			}
			break;
		}
	}
	scf_vector_add(bb->prevs, bb1);

	for (j  = 0; j < bb1->prevs->size; j++) {
		bb2 =        bb1->prevs->data[j];

		scf_list_t*        l;
		scf_list_t*        l2;
		scf_3ac_code_t*    c;
		scf_3ac_operand_t* dst;

		for (l  = scf_list_next(&bb2->list); l != &bb1->list && l != scf_list_sentinel(bb_list_head);
			 l  = scf_list_next(l)) {

			bb3 = scf_list_data(l, scf_basic_block_t, list);

			if (!bb3->jmp_flag)
				continue;

			for (l2 = scf_list_head(&bb3->code_list_head); l2 != scf_list_sentinel(&bb3->code_list_head);
				 l2 = scf_list_next(l2)) {

				c   = scf_list_data(l2, scf_3ac_code_t, list);
				dst = c->dsts->data[0];

				if (dst->bb == bb)
					dst->bb = bb1;
			}
		}

		scf_dn_status_t* ds2;
		int k;
		for (k  = 0; k < bb2->ds_malloced->size; k++) {
			ds2 =        bb2->ds_malloced->data[k];

			if (0 == scf_dn_status_cmp_same_dn_indexes(ds2, ds))
				continue;

			if (scf_vector_find_cmp(bb2->ds_freed, ds2, scf_dn_status_cmp_same_dn_indexes))
				continue;

			scf_dn_status_t* ds3 = scf_dn_status_clone(ds2);
			if (!ds3)
				return -ENOMEM;

			int ret = scf_vector_add(bb1->ds_malloced, ds3);
			if (ret < 0)
				return ret;
		}
	}

	int ret = _bb_add_gc_code(ast, bb1, ds, "scf_free");
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	return scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
}

static int _bb_split_prevs(scf_basic_block_t* bb, scf_dn_status_t* ds, scf_vector_t* bb_split_prevs)
{
	scf_basic_block_t* bb_prev;
	int i;

	for (i = 0; i < bb->prevs->size; i++) {
		bb_prev   = bb->prevs->data[i];

		if (!scf_vector_find_cmp(bb_prev->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes))
			continue;

		if (scf_vector_find_cmp(bb_prev->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
			continue;

		int ret = scf_vector_add(bb_split_prevs, bb_prev);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int _auto_gc_last_free(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	l  = scf_list_tail(bb_list_head);
	bb = scf_list_data(l, scf_basic_block_t, list);

	scf_logd("bb: %p, bb->ds_malloced->size: %d\n", bb, bb->ds_malloced->size);

	int i;
	for (i = 0; i < bb->ds_malloced->size; ) {

		scf_dn_status_t* ds = bb->ds_malloced->data[i];
		scf_dag_node_t*  dn = ds->dag_node;
		scf_variable_t*  v  = dn->var;

		scf_loge("f: %s, last free: v_%d_%d/%s, ds->ret: %u\n",
				f->node.w->text->data, v->w->line, v->w->pos, v->w->text->data, ds->ret);

		if (ds->ret) {
			i++;
			continue;
		}

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

		int ret = _bb_find_ds_malloced(bb, bb_list_head, ds, vec);
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

		ret = _bb_split_prevs(bb_dominator, ds, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}

		if (0 == vec->size || vec->size == bb_dominator->prevs->size) {

			if (scf_vector_find_cmp(bb_dominator->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes)) {

				ret = _bb_add_free(ast, f, bb_dominator, ds);
				i++;
			} else
				ret = _bb_add_free(ast, f, bb, ds);

			scf_vector_free(vec);
			vec = NULL;

			if (ret < 0)
				return ret;

		} else {
			ret = _bb_split_prev_add_free(ast, f, bb_dominator, bb_list_head, ds, vec);
			if (ret < 0)
				return ret;
			i++;
		}
	}

	return 0;
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
		SCF_XCHG(parent->ds_malloced, child->ds_malloced); \
		\
		scf_vector_free(child->exit_dn_actives); \
		scf_vector_free(child->exit_dn_aliases); \
		\
		child->exit_dn_actives = scf_vector_clone(parent->exit_dn_actives); \
		child->exit_dn_aliases = scf_vector_clone(parent->exit_dn_aliases); \
		\
		scf_list_add_front(&parent->list, &child->list); \
	} while (0)

static int _optimize_auto_gc_bb_ref(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t** pbb, scf_list_t* bb_list_head, scf_dn_status_t* ds)
{
	scf_basic_block_t* cur_bb = *pbb;
	scf_basic_block_t* bb1    = NULL;
	scf_dag_node_t*    dn     = ds->dag_node;

	AUTO_GC_BB_SPLIT(cur_bb, bb1);

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	int ret = _bb_add_gc_code(ast, bb1, ds, "scf_ref");
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	ret = scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
	if (ret < 0)
		return ret;

	bb1->call_flag  = 1;
	bb1->dereference_flag = 0;
	bb1->auto_ref_flag    = 1;

	*pbb = bb1;
	return 0;
}

static int _optimize_auto_gc_bb_free(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t** pbb, scf_list_t* bb_list_head, scf_dn_status_t* ds)
{
	scf_basic_block_t* cur_bb = *pbb;
	scf_basic_block_t* bb1    = NULL;
	scf_basic_block_t* bb2    = NULL;
	scf_dag_node_t*    dn     = ds->dag_node;

	AUTO_GC_BB_SPLIT(cur_bb, bb1);
	AUTO_GC_BB_SPLIT(bb1,    bb2);

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	bb1->call_flag  = 1;
	bb1->dereference_flag = 0;
	bb1->auto_free_flag   = 1;

	int ret = _bb_add_gc_code(ast, bb1, ds, "scf_free");
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	ret = scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
	if (ret < 0)
		return ret;

	*pbb = bb2;
	return 0;
}

static int _bb_prevs_malloced(scf_basic_block_t* bb, scf_vector_t* ds_malloced)
{
	scf_basic_block_t* bb_prev;
	int i;
	int j;

	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < bb_prev->ds_malloced->size; j++) {

			scf_dn_status_t* ds = bb_prev->ds_malloced->data[j];

			if (scf_vector_find_cmp(bb_prev->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
				continue;

			if (scf_vector_find_cmp(ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes))
				continue;

			scf_dn_status_t* ds2 = scf_dn_status_clone(ds);
			if (!ds2)
				return -ENOMEM;

			int ret = scf_vector_add(ds_malloced, ds2);
			if (ret < 0) {
				scf_dn_status_free(ds2);
				return ret;
			}
		}
	}
	return 0;
}


static int _optimize_auto_gc_bb(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;
	scf_vector_t*   ds_malloced;

	scf_basic_block_t* bb_prev = NULL;
	scf_basic_block_t* cur_bb  = bb;

	ds_malloced = scf_vector_alloc();
	if (!ds_malloced)
		return -ENOMEM;

	// at first, the malloced vars, are ones malloced in previous blocks

	int ret = _bb_prevs_malloced(bb, ds_malloced);
	if (ret < 0) {
		scf_vector_clear(ds_malloced, ( void (*)(void*) )scf_dn_status_free);
		scf_vector_free(ds_malloced);
		return ret;
	}

	scf_logd("bb: %p, ds_malloced->size: %d\n", bb, ds_malloced->size);

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_3ac_operand_t* dst;
		scf_3ac_operand_t* src;
		scf_dag_node_t*    dn;
		scf_dn_status_t*   ds_obj;
		scf_variable_t*    v0;

		if (SCF_OP_ASSIGN == c->op->type) {

			dst = c->dsts->data[0];
			v0  = dst->dag_node->var;

			if (!scf_variable_is_struct_pointer(v0))
				goto _end;

			ds_obj = scf_dn_status_alloc(dst->dag_node);
			if (!ds_obj)
				return -ENOMEM;

			src = c->srcs->data[0];
			dn  = src->dag_node;

		} else if (SCF_OP_3AC_ASSIGN_DEREFERENCE == c->op->type) {

			assert(2 == c->srcs->size);

			src = c->srcs->data[1];
			dn  = src->dag_node;
			v0  = dn->var;

			if (!scf_variable_is_struct_pointer(v0))
				goto _end;

			src     = c->srcs->data[0];
			ds_obj  = NULL;

			int ret = _ds_for_assign_dereference(&ds_obj, src->dag_node);
			if (ret < 0)
				return ret;
#if 0
		} else if (SCF_OP_RETURN == c->op->type) {

			src = c->srcs->data[0];
			dn  = src->dag_node;
			v0  = dn->var;

			if (!scf_variable_is_struct_pointer(v0))
				goto _end;

			ds_obj = scf_dn_status_alloc(src->dag_node);
			if (!ds_obj)
				return -ENOMEM;

			goto _ref;
#endif
		} else
			goto _end;

		if (scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {

			scf_vector_t* bb_split_prevs;

			bb_split_prevs = scf_vector_alloc();
			if (!bb_split_prevs)
				return -ENOMEM;

			int ret = _bb_split_prevs(bb, ds_obj, bb_split_prevs);
			if (ret < 0) {
				scf_vector_free(bb_split_prevs);
				bb_split_prevs = NULL;
				return ret;
			}

			if (bb_split_prevs->size > 0 && bb_split_prevs->size < bb->prevs->size) {

				scf_logw("auto gc error: bb: %p, not all v_%d_%d/%s in previous blocks malloced automaticly\n",
						bb, v0->w->line, v0->w->pos, v0->w->text->data);

				int ret = _bb_split_prev_add_free(ast, f, bb, bb_list_head, ds_obj, bb_split_prevs);
				if (ret < 0)
					return ret;

			} else {
				scf_vector_free(bb_split_prevs);
				bb_split_prevs = NULL;

				int ret = _optimize_auto_gc_bb_free(ast, f, &cur_bb, bb_list_head, ds_obj);
				if (ret < 0)
					return ret;

				scf_dn_status_t* ds = scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes);

				assert(ds);
				assert(0 == scf_vector_del(ds_malloced, ds));

				scf_dn_status_free(ds);
				ds = NULL;
			}
		}

_ref:
		if (cur_bb != bb)
			dn->var->local_flag = 1;

		while (dn) {
			if (SCF_OP_TYPE_CAST == dn->type)
				dn = dn->childs->data[0];

			else if (SCF_OP_EXPR == dn->type)
				dn = dn->childs->data[0];
			else
				break;
		}

		if (SCF_OP_CALL == dn->type) {

			scf_dag_node_t* dn_pf = dn->childs->data[0];
			scf_function_t* f2    = dn_pf->var->func_ptr;
			scf_variable_t* ret   = f2->rets->data[0];

			if (!strcmp(f2->node.w->text->data, "scf_malloc") || ret->auto_gc_flag) {

				if (SCF_OP_RETURN == c->op->type) {
					ret = f->rets->data[0];
					ret->auto_gc_flag = 1;
				}

#if 0
				else if (SCF_OP_3AC_ASSIGN_DEREFERENCE == c->op->type) {

					if (f->argv && scf_vector_find(f->argv, ds_obj->dag_node->var))
						ds_obj->dag_node->var->auto_gc_flag = 1;
				}
#endif
				if (!scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {

					assert(0 == scf_vector_add(ds_malloced, ds_obj));
				} else {
					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
				}

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}
				continue;
			}
		} else {
			scf_dn_status_t* ds = scf_dn_status_alloc(dn);
			if (!ds)
				return -ENOMEM;

			if (scf_vector_find_cmp(ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes)) {

				scf_basic_block_t* bb2 = NULL;

				if (SCF_OP_RETURN == c->op->type) {
					scf_variable_t* ret = f->rets->data[0];
					ret->auto_gc_flag = 1;
				}
#if 0
				else if (SCF_OP_3AC_ASSIGN_DEREFERENCE == c->op->type) {

					if (f->argv && scf_vector_find(f->argv, ds_obj->dag_node->var))
						ds_obj->dag_node->var->auto_gc_flag = 1;
				}
#endif

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				int ret = _optimize_auto_gc_bb_ref(ast, f, &cur_bb, bb_list_head, ds_obj);
				if (ret < 0)
					return ret;

				if (!scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {

					assert(0 == scf_vector_add(ds_malloced, ds_obj));
				} else {
					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
				}

				if (l != scf_list_sentinel(&bb->code_list_head)) {

					AUTO_GC_BB_SPLIT(cur_bb, bb2);
					cur_bb = bb2;
				}

				scf_dn_status_free(ds);
				ds = NULL;
				continue;
			}

			scf_dn_status_free(ds);
			ds = NULL;
		}

		scf_dn_status_free(ds_obj);
		ds_obj = NULL;
_end:
		if (cur_bb != bb) {
			scf_list_del(&c->list);
			scf_list_add_tail(&cur_bb->code_list_head, &c->list);
		}
	}

	return 0;
}

static int _optimize_auto_gc(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!ast || !f || !bb_list_head)
		return -EINVAL;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;

	int ret;

	ret = _auto_gc_last_free(ast, f, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		scf_dag_node_t* dn;
		scf_vector_t*   dn_actives;

		scf_list_t*     start = l;
		scf_list_t*     l2;

		bb = scf_list_data(l, scf_basic_block_t, list);


		for (l  = scf_list_next(l); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

			bb2 = scf_list_data(l, scf_basic_block_t, list);

			if (!bb2->auto_ref_flag && !bb2->auto_free_flag)
				break;
		}


		ret = _optimize_auto_gc_bb(ast, f, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		dn_actives = scf_vector_alloc();
		if (!dn_actives)
			return -ENOMEM;

		for (l2 = scf_list_prev(l); l2 != scf_list_prev(start); l2 = scf_list_prev(l2)) {

			bb  = scf_list_data(l2, scf_basic_block_t, list);

			ret = scf_basic_block_active_vars(bb);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			int i;
			for (i = 0; i < dn_actives->size; i++) {
				dn =        dn_actives->data[i];

				ret = scf_vector_add_unique(bb->exit_dn_actives, dn);
				if (ret < 0) {
					scf_vector_free(dn_actives);
					return ret;
				}
			}

			for (i = 0; i < bb->entry_dn_actives->size; i++) {
				dn =        bb->entry_dn_actives->data[i];

				ret = scf_vector_add_unique(dn_actives, dn);
				if (ret < 0) {
					scf_vector_free(dn_actives);
					return ret;
				}
			}
		}

		scf_vector_free(dn_actives);
		dn_actives = NULL;

		for (l2 = scf_list_prev(l); l2 != scf_list_prev(start); ) {

			bb  = scf_list_data(l2, scf_basic_block_t, list);
			l2  = scf_list_prev(l2);
#if 1
			if (l2 != start && scf_list_prev(l) != &bb->list) {

				bb2 = scf_list_data(l2, scf_basic_block_t, list);

				if (bb->auto_free_flag
						&& bb2->auto_ref_flag
						&& 0 == scf_dn_status_cmp_same_dn_indexes(bb->ds_auto_gc, bb2->ds_auto_gc)) {

					scf_basic_block_t* bb3;
					scf_basic_block_t* bb4;

					assert(1 == bb2->prevs->size);
					assert(1 == bb ->nexts->size);

					bb3 = bb2->prevs->data[0];
					bb4 = bb ->nexts->data[0];

					assert(1 == bb3->nexts->size);
					assert(1 == bb4->prevs->size);

					bb3->nexts->data[0] = bb4;
					bb4->prevs->data[0] = bb3;

					l2 = scf_list_prev(l2);

					scf_list_del(&bb->list);
					scf_list_del(&bb2->list);

					scf_basic_block_free(bb);
					scf_basic_block_free(bb2);
					bb  = NULL;
					bb2 = NULL;
					continue;
				}
			}
#endif
			ret = scf_basic_block_loads_saves(bb, bb_list_head);
			if (ret < 0)
				return ret;
		}
	}

//	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_auto_gc =
{
	.name     =  "auto_gc",

	.optimize =  _optimize_auto_gc,
};

