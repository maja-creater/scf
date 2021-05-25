#include"scf_optimizer.h"

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

static int _bb_add_ds(scf_basic_block_t* bb, scf_dn_status_t* ds_obj)
{
	scf_dn_status_t*   ds2;

	ds2 = scf_vector_find_cmp(bb->ds_freed, ds_obj, scf_dn_status_cmp_same_dn_indexes);
	if (ds2) {
		scf_vector_del(bb->ds_freed, ds2);
		if (ds2 != ds_obj)
			scf_dn_status_free(ds2);
		ds2 = NULL;
	}

	ds2 = scf_vector_find_cmp(bb->ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes);
	if (!ds2) {
		assert(0 == scf_vector_add(bb->ds_malloced, ds_obj));

	} else {
		ds2->ret = ds_obj->ret;

		scf_dn_status_free(ds_obj);
		ds_obj = NULL;
	}

	return 0;
}

static int _bb_del_ds(scf_basic_block_t* bb, scf_dn_status_t* ds_obj)
{
	scf_dn_status_t*   ds2;

	if (!scf_vector_find_cmp(bb->ds_freed, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {

		ds2 = scf_dn_status_clone(ds_obj);
		assert(ds2);

		assert(0 == scf_vector_add(bb->ds_freed, ds2));
	}

	ds2 = scf_vector_find_cmp(bb->ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes);
	if (ds2) {
		scf_vector_del(bb->ds_malloced, ds2);
		if (ds2 != ds_obj)
			scf_dn_status_free(ds2);
		ds2 = NULL;
	}

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

		scf_3ac_operand_t* dst;
		scf_3ac_operand_t* src;
		scf_dn_status_t*   ds_obj;
		scf_dn_status_t*   ds;
		scf_dn_status_t*   ds2;
		scf_dag_node_t*    dn;
		scf_variable_t*    v0;

		if (SCF_OP_ASSIGN == c->op->type) {

			dst = c->dsts->data[0];
			v0  = dst->dag_node->var;

			if (!scf_variable_is_struct_pointer(v0))
				continue;

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
				continue;

			src     = c->srcs->data[0];
			ds_obj  = NULL;

			int ret = _ds_for_assign_dereference(&ds_obj, src->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;
#if 1
		} else if (SCF_OP_RETURN == c->op->type) {

			src = c->srcs->data[0];
			dn  = src->dag_node;
			v0  = dn->var;

			if (!scf_variable_is_struct_pointer(v0))
				continue;

			ds_obj = scf_dn_status_alloc(dn);
			if (!ds_obj)
				return -ENOMEM;

			ds_obj->ret = 1;
			goto ref;
#endif
		} else if (SCF_OP_CALL == c->op->type) {

			assert(c->srcs->size > 0);
			src =  c->srcs->data[0];

			scf_dag_node_t* dn_pf = src->dag_node;
			scf_function_t* f2    = dn_pf->var->func_ptr;

			int i;
			for (i  = 1; i < c->srcs->size; i++) {

				src = c->srcs->data[i];
				dn  = src->dag_node;
				v0  = dn->var;

				if (scf_variable_nb_pointers(v0) < 2)
					continue;

				scf_variable_t* v1 = f2->argv->data[i - 1];

				if (!v1->auto_gc_flag)
					continue;

				while (dn) {
					if (SCF_OP_TYPE_CAST == dn->type)
						dn = dn->childs->data[0];

					else if (SCF_OP_EXPR == dn->type)
						dn = dn->childs->data[0];
					else
						break;
				}

				ds_obj  = NULL;

				int ret;
				if (SCF_OP_ADDRESS_OF == dn->type)

					ret = _ds_for_dn(&ds_obj, dn->childs->data[0]);
				else
					ret = _ds_for_assign_dereference(&ds_obj, dn);

				if (ret < 0)
					return ret;

//				scf_loge("bb: %p\n", bb);
//				scf_dn_status_print(ds_obj);
				if (ds_obj->dag_node->var->arg_flag)
					ds_obj->ret = 1;

				if (scf_vector_add_unique(bb->dn_reloads, ds_obj->dag_node) < 0) {
					scf_loge("\n");

					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
					return -ENOMEM;
				}

				_bb_add_ds(bb, ds_obj);
				count++;
			}
			continue;
		} else
			continue;

		_bb_del_ds(bb, ds_obj);
		count++;

ref:
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

				_bb_add_ds(bb, ds_obj);
				count++;
				continue;
			}
		} else {
			ds = scf_dn_status_alloc(dn);
			assert(ds);

			if (scf_vector_find_cmp(bb->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes)
					&& !scf_vector_find_cmp(bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes)) {

				_bb_add_ds(bb, ds_obj);
				count++;
				continue;
			}
		}

		scf_dn_status_free(ds_obj);
		ds_obj = NULL;
	}

	return count;
}

static int _auto_gc_bb_next_find(scf_basic_block_t* bb, void* data, scf_vector_t* queue)
{
	scf_basic_block_t* next_bb;
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		next_bb   = bb->nexts->data[j];

		int k;
		for (k = 0; k < bb->ds_malloced->size; k++) {
			ds =        bb->ds_malloced->data[k];

			if (scf_vector_find_cmp(bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
				continue;

			if (scf_vector_find_cmp(next_bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
				continue;

			ds2 = scf_vector_find_cmp(next_bb->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes);
			if (ds2) {
				uint32_t tmp = ds2->ret;

				ds2->ret |= ds->ret;

				if (tmp != ds2->ret)
					count++;
				continue;
			}

			ds2 = scf_dn_status_clone(ds);
			if (!ds2)
				return -ENOMEM;

			ret = scf_vector_add(next_bb->ds_malloced, ds2);
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

static int _bfs_sort_function(scf_vector_t* fqueue, scf_function_t* fmalloc)
{
	scf_function_t* f;
	scf_function_t* f2;

	int ret = scf_vector_add(fqueue, fmalloc);
	if (ret < 0)
		return ret;

	int i;
	for (i = 0; i < fqueue->size; i++) {
		f  =        fqueue->data[i];

		if (f->visited_flag)
			continue;

		scf_loge("f: %p, %s\n", f, f->node.w->text->data);

		f->visited_flag = 1;

		int j;
		for (j = 0; j < f->caller_functions->size; j++) {
			f2 =        f->caller_functions->data[j];

			if (f2->visited_flag)
				continue;

			ret = scf_vector_add(fqueue, f2);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int _auto_gc_function_find(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_dn_status_t*   ds;

	int total = 0;
	int count;
	int ret;

	do {
		for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

			bb = scf_list_data(l, scf_basic_block_t, list);
			l  = scf_list_next(l);

			ret = _auto_gc_bb_find(bb);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			total += ret;
		}

		l   = scf_list_head(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);

		ret = scf_basic_block_search_bfs(bb, _auto_gc_bb_next_find, NULL);
		if (ret < 0)
			return ret;

		total += ret;
		count  = ret;
	} while (count > 0);

	l   = scf_list_tail(bb_list_head);
	bb  = scf_list_data(l, scf_basic_block_t, list);

	int i;
	for (i = 0; i < bb->ds_malloced->size; i++) {
		ds =        bb->ds_malloced->data[i];

		scf_loge("ds->ret: %u, ds->dag_node->var->arg_flag: %u\n", ds->ret, ds->dag_node->var->arg_flag);
		scf_dn_status_print(ds);
		printf("\n");

		if (!ds->ret)
			continue;

		if (ds->dag_node->var->arg_flag)
			ds->dag_node->var->auto_gc_flag = 1;
		else {
			scf_variable_t* ret = f->rets->data[0];
			ret->auto_gc_flag = 1;
		}
	}

	return total;
}

static int _auto_gc_global_find(scf_ast_t* ast, scf_vector_t* functions)
{
	scf_function_t* fmalloc = NULL;
	scf_function_t* f;
	scf_vector_t*   fqueue;

	int i;
	for (i = 0; i < functions->size; i++) {
		f  =        functions->data[i];

		f->visited_flag = 0;

		if (!fmalloc && !strcmp(f->node.w->text->data, "scf_malloc"))
			fmalloc = f;
	}

	if (!fmalloc)
		return 0;

	fqueue = scf_vector_alloc();
	if (!fqueue)
		return -ENOMEM;

	int ret = _bfs_sort_function(fqueue, fmalloc);
	if (ret < 0) {
		scf_vector_free(fqueue);
		return ret;
	}

	int total0 = 0;
	int total1 = 0;

	do {
		total0 = total1;
		total1 = 0;

		for (i = 0; i < fqueue->size; i++) {
			f  =        fqueue->data[i];

			if (!f->node.define_flag)
				continue;

			ret = _auto_gc_function_find(ast, f, &f->basic_block_list_head);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			total1 += ret;
		}

		scf_loge("total0: %d, total1: %d\n", total0, total1);

	} while (total0 != total1);

	return 0;
}

static int _optimize_auto_gc_find(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!ast || !functions || functions->size <= 0)
		return -EINVAL;

	int ret = _auto_gc_global_find(ast, functions);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_auto_gc_find =
{
	.name     =  "auto_gc_find",

	.optimize =  _optimize_auto_gc_find,
};

