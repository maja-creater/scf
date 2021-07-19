#include"scf_optimizer.h"

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

		ds2 = scf_vector_find_cmp(bb->ds_malloced, ds_obj, scf_dn_status_cmp_like_dn_indexes);
		if (!ds2) {
			assert(0 == scf_vector_add(bb->ds_malloced, ds_obj));
			return 0;
		}
	}

	ds2->ret |= ds_obj->ret;

	scf_dn_status_free(ds_obj);
	return 0;
}

static int _bb_del_ds(scf_basic_block_t* bb, scf_dn_status_t* ds_obj)
{
	scf_dn_status_t*   ds2;

	if (!scf_vector_find_cmp(bb->ds_freed, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {

		ds2 = scf_vector_find_cmp(bb->ds_freed, ds_obj, scf_dn_status_cmp_like_dn_indexes);
		if (!ds2) {
			ds2 = scf_dn_status_clone(ds_obj);
			assert(ds2);

			assert(0 == scf_vector_add(bb->ds_freed, ds2));
		}
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

static int _bb_add_ds_for_call(scf_basic_block_t* bb, scf_dn_status_t* ds_obj, scf_function_t* f2, scf_variable_t* arg)
{
	scf_list_t*        l2  = scf_list_tail(&f2->basic_block_list_head);
	scf_basic_block_t* bb2 = scf_list_data(l2, scf_basic_block_t, list);

	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;

	scf_dn_index_t*    di;
	scf_dn_index_t*    di2;

	int i;
	for (i  = 0; i < bb2->ds_malloced->size; i++) {
		ds2 =        bb2->ds_malloced->data[i];

		if (ds2->dag_node->var != arg)
			continue;

		ds = scf_dn_status_clone(ds_obj);
		if (!ds)
			return -ENOMEM;

		if (!ds2->dn_indexes) {
			_bb_add_ds(bb, ds);
			continue;
		}

		if (!ds->dn_indexes) {
			ds ->dn_indexes = scf_vector_alloc();

			if (!ds->dn_indexes) {
				scf_dn_status_free(ds);
				return -ENOMEM;
			}
		}

		int size = ds->dn_indexes->size;
		int j;
		for (j  = 0; j < ds2->dn_indexes->size; j++) {
			di2 =        ds2->dn_indexes->data[j];

			di = scf_dn_index_alloc();
			if (!di) {
				scf_dn_status_free(ds);
				return -ENOMEM;
			}
			di->index  = di2->index;
			di->member = di2->member;

			if (di2->dn) {
				di ->dn = scf_dag_node_alloc(di2->dn->type, di2->dn->var, di2->dn->node);
				if (!di->dn) {
					scf_dn_status_free(ds);
					scf_dn_index_free (di);
				}
			}

			if (scf_vector_add(ds->dn_indexes, di) < 0) {
				scf_dn_status_free(ds);
				scf_dn_index_free (di);
				return -ENOMEM;
			}
		}

		int k;
		for (k = 0; k < ds2->dn_indexes->size; k++) {

			di = ds->dn_indexes->data[size + k];

			for (j = size + k - 1; j > k; j--)
				ds->dn_indexes->data[j + 1] = ds->dn_indexes->data[j];

			ds->dn_indexes->data[k] = di;
		}

		_bb_add_ds(bb, ds);
	}

	return 0;
}

static int _auto_gc_bb_find(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	int count = 0;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_3ac_operand_t* base;
		scf_3ac_operand_t* member;
		scf_3ac_operand_t* index;
		scf_3ac_operand_t* scale;

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

			if (!scf_variable_may_malloced(v0))
				continue;

			ds_obj = scf_dn_status_alloc(dst->dag_node);
			if (!ds_obj)
				return -ENOMEM;

			src = c->srcs->data[0];
			dn  = src->dag_node;

		} else if (SCF_OP_3AC_ASSIGN_ARRAY_INDEX == c->op->type) {

			assert(4 == c->srcs->size);

			base  = c->srcs->data[0];
			index = c->srcs->data[1];
			scale = c->srcs->data[2];
			src   = c->srcs->data[3];
			dn    = src->dag_node;
			v0    = _scf_operand_get(base->node->parent);

			if (!scf_variable_may_malloced(v0))
				continue;

			ds_obj = NULL;

			int ret = scf_ds_for_assign_array_member(&ds_obj, base->dag_node, index->dag_node, scale->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;

		} else if (SCF_OP_3AC_ASSIGN_POINTER == c->op->type) {

			assert(3 == c->srcs->size);

			base   = c->srcs->data[0];
			member = c->srcs->data[1];
			src    = c->srcs->data[2];
			dn     = src->dag_node;
			v0     = member->dag_node->var;

			if (!scf_variable_may_malloced(v0))
				continue;

			ds_obj = NULL;

			int ret = scf_ds_for_assign_member(&ds_obj, base->dag_node, member->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;

		} else if (SCF_OP_3AC_ASSIGN_DEREFERENCE == c->op->type) {

			assert(2 == c->srcs->size);

			base = c->srcs->data[0];
			src  = c->srcs->data[1];
			dn   = src->dag_node;
			v0   = _scf_operand_get(base->node->parent);

			if (!scf_variable_may_malloced(v0))
				continue;

			ds_obj  = NULL;

			int ret = scf_ds_for_assign_dereference(&ds_obj, base->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;
#if 1
		} else if (SCF_OP_RETURN == c->op->type) {

			src = c->srcs->data[0];
			dn  = src->dag_node;
			v0  = dn->var;

			if (!scf_variable_may_malloced(v0))
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

				if (v0->nb_pointers + v0->nb_dimentions + (v0->type >= SCF_STRUCT) < 2)
					continue;

				if (i - 1 >= f2->argv->size)
					continue;

				scf_variable_t* v1 = f2->argv->data[i - 1];

				if (!v1->auto_gc_flag)
					continue;

				scf_logd("f2: %s, v0: %s, v1: %s\n",
						f2->node.w->text->data, v0->w->text->data, v1->w->text->data);

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

					ret = scf_ds_for_dn(&ds_obj, dn->childs->data[0]);
				else
					ret = scf_ds_for_dn(&ds_obj, dn);

				if (ret < 0)
					return ret;

				if (ds_obj->dag_node->var->arg_flag)
					ds_obj->ret = 1;

				if (scf_vector_add_unique(bb->dn_reloads, ds_obj->dag_node) < 0) {

					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
					return -ENOMEM;
				}

				ret = _bb_add_ds_for_call(bb, ds_obj, f2, v1);

				scf_dn_status_free(ds_obj);
				ds_obj = NULL;
				if (ret < 0)
					return ret;

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

		if (SCF_OP_CALL == dn->type || dn->node->split_flag) {

			if (dn->node->split_flag) {
				assert(SCF_OP_CALL   == dn->node->split_parent->type
					|| SCF_OP_CREATE == dn->node->split_parent->type);
			}

			scf_variable_t* v = dn->var;
			if (v->w)
				scf_loge("dn: %p, type: %d, v: %d/%s\n", dn, dn->type, v->w->line, v->w->text->data);

			scf_dag_node_t* dn_pf = dn->childs->data[0];
			scf_function_t* f2    = dn_pf->var->func_ptr;
			scf_variable_t* ret   = f2->rets->data[0];

			scf_logd("f2: %s, ret->auto_gc_flag: %d\n", f2->node.w->text->data, ret->auto_gc_flag);

			if (!strcmp(f2->node.w->text->data, "scf__auto_malloc") || ret->auto_gc_flag) {

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

			ds2 = scf_vector_find_cmp(next_bb->ds_malloced, ds, scf_dn_status_cmp_like_dn_indexes);
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

			ret = _auto_gc_bb_find(bb, f);
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

		if (!fmalloc && !strcmp(f->node.w->text->data, "scf__auto_malloc"))
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

			if (!strcmp(f->node.w->text->data, "scf__auto_malloc"))
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

