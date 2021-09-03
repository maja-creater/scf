#include"scf_optimizer.h"
#include"scf_pointer_alias.h"

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

static int __bb_add_ds(scf_basic_block_t* bb, scf_dn_status_t* ds_obj, scf_basic_block_t* __bb, scf_dn_status_t* __ds)
{
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;

	scf_dn_index_t*    di;
	scf_dn_index_t*    di2;

	int i;
	for (i  = 0; i < __bb->ds_malloced->size; i++) {
		ds2 =        __bb->ds_malloced->data[i];

		if (ds2->dag_node->var != __ds->dag_node->var)
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

static int _bb_add_ds_for_ret(scf_basic_block_t* bb, scf_dn_status_t* ds_obj, scf_function_t* f2, scf_3ac_code_t* c)
{
	scf_list_t*        l2  = scf_list_tail(&f2->basic_block_list_head);
	scf_basic_block_t* bb2 = scf_list_data(l2, scf_basic_block_t, list);
	scf_dn_status_t*   ds2;

	int i;
	for (i  = 0; i < bb2->ds_malloced->size; i++) {
		ds2 =        bb2->ds_malloced->data[i];

		if (!ds2->ret)
			continue;

		__bb_add_ds(bb, ds_obj, bb2, ds2);
	}

	return 0;
}

int __bb_find_ds_alias(scf_vector_t* aliases, scf_dn_status_t* ds_obj, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_obj2;
	scf_dn_status_t* ds_alias;
	scf_dn_index_t*  di;

	int ret;
	int i;
	int ndi = 0;

	ds_obj2 = scf_dn_status_clone(ds_obj);
	if (!ds_obj2)
		return -ENOMEM;

	while (1) {
		scf_vector_t*    aliases2;
		scf_dn_status_t* ds_alias2;

		scf_3ac_code_t*  c2 = scf_list_data(scf_list_prev(&c->list), scf_3ac_code_t, list);

		aliases2 = scf_vector_alloc();
		if (!aliases2)
			return -ENOMEM;

		ret = scf_pointer_alias_ds(aliases2, ds_obj2, c2, bb, bb_list_head);
		if (ret < 0) {
			scf_vector_free(aliases2);

			if (SCF_POINTER_NOT_INIT == ret)
				break;
			return ret;
		}

		int j;
		for (j = 0; j < aliases2->size; j++) {
			ds_alias2 = aliases2->data[j];

			SCF_XCHG(ds_alias2->dn_indexes, ds_alias2->alias_indexes);
			SCF_XCHG(ds_alias2->dag_node,   ds_alias2->alias);

			if (ds_obj->dn_indexes) {

				if (!ds_alias2->dn_indexes) {
					ds_alias2 ->dn_indexes = scf_vector_alloc();
					if (!ds_alias2->dn_indexes)
						return -ENOMEM;
				}

				int k;
				for (k = 0; k < ndi; k++) {
					di = ds_obj->dn_indexes->data[k];

					ret = scf_vector_add(ds_alias2->dn_indexes, di);
					if (ret < 0) {
						scf_loge("\n");
						return ret;
					}
					di->refs++;
				}

				for (k = ds_alias2->dn_indexes->size - 2; k >= 0; k--)
					ds_alias2->dn_indexes->data[k + 1] = ds_alias2->dn_indexes->data[k];

				for (k = 0; k < ndi; k++)
					ds_alias2->dn_indexes->data[k] = ds_obj->dn_indexes->data[k];
			}

			if (scf_vector_add(aliases, ds_alias2) < 0) {
				scf_vector_free (aliases2);
				return ret;
			}
		}

		scf_vector_free (aliases2);
		aliases2 = NULL;

		if (ds_obj2->dn_indexes) {

			assert(ds_obj2->dn_indexes->size > 0);

			di = ds_obj2->dn_indexes->data[0];

			for (j = 1; j < ds_obj2->dn_indexes->size; j++)
				ds_obj2->dn_indexes->data[j - 1] = ds_obj2->dn_indexes->data[j];

			scf_dn_index_free(di);
			di = NULL;

			ndi++;

			ds_obj2->dn_indexes->size--;

			if (0 == ds_obj2->dn_indexes->size) {
				scf_vector_free(ds_obj2->dn_indexes);
				ds_obj2->dn_indexes = NULL;
			}
		} else
			break;
	}

	return 0;
}

static int _bb_find_ds_alias(scf_dn_status_t* ds_obj, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_obj2;
	scf_dn_status_t* ds_alias;
	scf_dn_index_t*  di;
	scf_vector_t*    aliases;

	int ret;
	int i;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __bb_find_ds_alias(aliases, ds_obj, c, bb, bb_list_head);
	if (ret < 0)
		return ret;

	int need = 0;
	for (i = 0; i < aliases->size; i++) {
		ds_alias  = aliases->data[i];

		if (!ds_alias->dag_node)
			continue;

		if (scf_vector_find_cmp(bb->ds_malloced, ds_alias, scf_dn_status_cmp_same_dn_indexes)
				&& !scf_vector_find_cmp(bb->ds_freed, ds_alias, scf_dn_status_cmp_same_dn_indexes)) {
			need = 1;
			break;
		}
	}

	if (scf_vector_find_cmp(bb->ds_malloced, ds_obj, scf_dn_status_cmp_same_dn_indexes)
			&& !scf_vector_find_cmp(bb->ds_freed, ds_obj, scf_dn_status_cmp_same_dn_indexes)) {
		need = 1;
	}

	ret = need;

error:
//	scf_vector_clear(aliases, ( void (*)(void*) ) scf_dn_status_free);
	scf_vector_free (aliases);
	return ret;
}

#define AUTO_GC_FIND_BB_SPLIT(parent, child) \
	do { \
		int ret = scf_basic_block_split(parent, &child); \
		if (ret < 0) \
			return ret; \
		\
		child->call_flag        = parent->call_flag; \
		child->dereference_flag = parent->dereference_flag; \
		\
		scf_vector_free(child->exit_dn_actives); \
		scf_vector_free(child->exit_dn_aliases); \
		scf_vector_free(child->dn_loads);   \
		scf_vector_free(child->dn_reloads); \
		\
		child->exit_dn_actives = scf_vector_clone(parent->exit_dn_actives); \
		child->exit_dn_aliases = scf_vector_clone(parent->exit_dn_aliases); \
		child->dn_loads        = scf_vector_clone(parent->dn_loads);   \
		child->dn_reloads      = scf_vector_clone(parent->dn_reloads); \
		\
		scf_list_add_front(&parent->list, &child->list); \
	} while (0)

static int _auto_gc_bb_find(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	scf_basic_block_t* cur_bb = bb;
	scf_basic_block_t* bb2    = NULL;

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
				goto _end;

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
				goto _end;

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
				goto _end;

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
				goto _end;

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
				goto _end;

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
			scf_variable_t* ret   = f2->rets->data[0];

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

				if (scf_vector_add_unique(cur_bb->dn_reloads, ds_obj->dag_node) < 0) {

					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
					return -ENOMEM;
				}

				ret = _bb_add_ds_for_call(cur_bb, ds_obj, f2, v1);

				scf_dn_status_free(ds_obj);
				ds_obj = NULL;
				if (ret < 0)
					return ret;

				count++;
			}

			if (ret->auto_gc_flag) {
				dst = c->dsts->data[0];
				v0  = dst->dag_node->var;

				if (!scf_variable_may_malloced(v0))
					goto _end;

				ds_obj = scf_dn_status_alloc(dst->dag_node);
				if (!ds_obj)
					return -ENOMEM;

				_bb_add_ds(cur_bb, ds_obj);
			}
			goto _end;
		} else
			goto _end;

		_bb_del_ds(cur_bb, ds_obj);
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
				scf_logd("dn: %p, type: %d, v: %d/%s\n", dn, dn->type, v->w->line, v->w->text->data);

			scf_dag_node_t* dn_pf = dn->childs->data[0];
			scf_function_t* f2    = dn_pf->var->func_ptr;
			scf_variable_t* ret   = f2->rets->data[0];

			scf_logd("f2: %s, ret->auto_gc_flag: %d\n", f2->node.w->text->data, ret->auto_gc_flag);

			if (!strcmp(f2->node.w->text->data, "scf__auto_malloc")) {
				_bb_add_ds(cur_bb, ds_obj);
				count++;
				goto _end;

			} else if (ret->auto_gc_flag) {

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				_bb_add_ds(cur_bb, ds_obj);
				_bb_add_ds_for_ret(cur_bb, ds_obj, f2, c);

				ds = scf_dn_status_alloc(dn);
				if (!ds)
					return -ENOMEM;

				_bb_del_ds(cur_bb, ds);
				ds = NULL;

				if (l != scf_list_sentinel(&bb->code_list_head)) {

					AUTO_GC_FIND_BB_SPLIT(cur_bb, bb2);
					cur_bb = bb2;
					scf_vector_add_unique(cur_bb->dn_reloads, ds_obj->dag_node);
				}

				count++;
				continue;
			}
		} else {
			ds  = NULL;
			int ret = scf_ds_for_dn(&ds, dn);
			if (ret < 0)
				return ret;

			ret = _bb_find_ds_alias(ds, c, bb, &f->basic_block_list_head);
			if (ret < 0)
				return ret;

			if (1 == ret) {
				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				_bb_add_ds(cur_bb, ds_obj);

				if (l != scf_list_sentinel(&bb->code_list_head)) {

					AUTO_GC_FIND_BB_SPLIT(cur_bb, bb2);
					cur_bb = bb2;
					scf_vector_add_unique(cur_bb->dn_reloads, ds_obj->dag_node);
				}

				count++;
				continue;
			}
		}

		scf_dn_status_free(ds_obj);
		ds_obj = NULL;

_end:
		if (cur_bb != bb) {
			scf_list_del(&c->list);
			scf_list_add_tail(&cur_bb->code_list_head, &c->list);
		}
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

static int _bb_find_ds_alias_leak(scf_dn_status_t* ds_obj, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_obj2;
	scf_dn_status_t* ds_alias;
	scf_dn_index_t*  di;
	scf_vector_t*    aliases;

	int ret;
	int i;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = scf_pointer_alias_ds_leak(aliases, ds_obj, c, bb, bb_list_head);
	if (ret < 0) {
		scf_vector_free(aliases);
		return ret;
	}

	int need = 0;
	for (i = 0; i < aliases->size; i++) {
		ds_alias  = aliases->data[i];

		SCF_XCHG(ds_alias->dn_indexes, ds_alias->alias_indexes);
		SCF_XCHG(ds_alias->dag_node,   ds_alias->alias);

		if (!ds_alias->dag_node)
			continue;

		ret = __bb_add_ds(bb, ds_obj, bb, ds_alias);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	ret = need;
error:
	scf_vector_free (aliases);
	return ret;
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
	scf_3ac_code_t*    c;

	int total = 0;
	int count;
	int ret;

	scf_loge("f: %s\n", f->node.w->text->data);

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

	l   = scf_list_tail(&bb->code_list_head);
	c   = scf_list_data(l, scf_3ac_code_t, list);

	int i;
	for (i = 0; i < bb->ds_malloced->size; i++) {
		ds =        bb->ds_malloced->data[i];
#if 1
		scf_loge("ds->ret: %u, ds->dag_node->var->arg_flag: %u\n", ds->ret, ds->dag_node->var->arg_flag);
		scf_dn_status_print(ds);
		printf("\n");
#endif
		if (!ds->ret)
			continue;

		if (ds->dag_node->var->arg_flag)
			ds->dag_node->var->auto_gc_flag = 1;
		else {
			scf_variable_t* ret = f->rets->data[0];
			ret->auto_gc_flag = 1;

			_bb_find_ds_alias_leak(ds, c, bb, bb_list_head);
		}
	}
	scf_loge("f: %s *****\n\n", f->node.w->text->data);

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

	.flags    = SCF_OPTIMIZER_GLOBAL,
};

