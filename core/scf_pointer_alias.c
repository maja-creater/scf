#include"scf_optimizer.h"
#include"scf_pointer_alias.h"

static int __bb_dfs_initeds(scf_basic_block_t* root, scf_active_var_t* ds, scf_vector_t* initeds)
{
	scf_basic_block_t* bb;
	scf_active_var_t*  status;

	int i;
	int j;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	for (i = 0; i < root->prevs->size; ++i) {

		bb = root->prevs->data[i];

		if (bb->visited_flag)
			continue;

		for (j = 0; j < bb->dn_status_initeds->size; j++) {

			status = bb->dn_status_initeds->data[j];

			if (0 == scf_dn_status_cmp_same_dn_indexes(ds, status))
				break;
		}

		if (j < bb->dn_status_initeds->size) {
			ret = scf_vector_add(initeds, bb);
			if (ret < 0)
				return ret;

			bb->visited_flag = 1;
			continue;
		}

		ret = __bb_dfs_initeds(bb, ds, initeds);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

static int __bb_dfs_check_initeds(scf_basic_block_t* root, scf_basic_block_t* obj)
{
	scf_basic_block_t* bb;

	int i;
	int ret;

	assert(!root->jmp_flag);

	if (root == obj)
		return -1;

	if (root->visited_flag)
		return 0;

	root->visited_flag = 1;

	for (i = 0; i < root->nexts->size; ++i) {

		bb = root->nexts->data[i];

		if (bb->visited_flag)
			continue;

		if (bb == obj)
			return -1;

		ret = __bb_dfs_check_initeds(bb, obj);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

static int _bb_dfs_initeds(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_active_var_t* ds, scf_vector_t* initeds)
{
	scf_list_t*        l;
	scf_basic_block_t* bb2;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb2 = scf_list_data(l, scf_basic_block_t, list);

		bb2->visited_flag = 0;
	}

	return __bb_dfs_initeds(bb, ds, initeds);
}

static int _bb_dfs_check_initeds(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_vector_t* initeds)
{
	scf_list_t*        l;
	scf_basic_block_t* bb2;

	int i;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb2 = scf_list_data(l, scf_basic_block_t, list);

		bb2->visited_flag = 0;
	}

	for (i = 0; i < initeds->size; i++) {
		bb2 = initeds->data[i];
		bb2->visited_flag = 1;
	}

	l = scf_list_head(bb_list_head);
	bb2 = scf_list_data(l, scf_basic_block_t, list);

	return __bb_dfs_check_initeds(bb2, bb);
}

static int _bb_pointer_initeds(scf_vector_t* initeds, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_active_var_t* ds)
{
	scf_variable_t* v;
	int ret;

	ret = _bb_dfs_initeds(bb_list_head, bb, ds, initeds);
	if (ret < 0)
		return ret;

	if (0 == initeds->size) {
		v = ds->dag_node->var;
		scf_loge("pointer v_%d_%d/%s is not inited\n", v->w->line, v->w->pos, v->w->text->data);
		return -1;
	}

	ret = _bb_dfs_check_initeds(bb_list_head, bb, initeds);
	if (ret < 0) {
		v = ds->dag_node->var;
		scf_loge("in bb: %p, pointer v_%d_%d/%s may not be inited\n",
				bb, v->w->line, v->w->pos, v->w->text->data);
	}
	return ret;
}

static int _bb_pointer_aliases(scf_vector_t* aliases, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_active_var_t* ds_pointer)
{
	scf_vector_t*      initeds;
	scf_basic_block_t* bb2;
	scf_active_var_t*  status;

	int ret;
	int i;
	int j;

	initeds = scf_vector_alloc();
	if (!initeds)
		return -ENOMEM;

	ret = _bb_pointer_initeds(initeds, bb_list_head, bb, ds_pointer);
	if (ret < 0) {
		scf_vector_free(initeds);
		return ret;
	}

	for (i = 0; i < initeds->size; i++) {
		bb2       = initeds->data[i];

		for (j = 0; j < bb2->dn_status_initeds->size; j++) {
			status    = bb2->dn_status_initeds->data[j];

			if (scf_dn_status_cmp_like_dn_indexes(status, ds_pointer))
				continue;

			if (scf_vector_find_cmp(aliases, status, scf_dn_status_cmp_alias))
				continue;

			ret = scf_vector_add(aliases, status);
			if (ret < 0) {
				scf_vector_free(initeds);
				return ret;
			}
		}
	}

	scf_vector_free(initeds);
	return 0;
}

int _bb_copy_aliases(scf_basic_block_t* bb, scf_dag_node_t* dn_pointer, scf_dag_node_t* dn_dereference, scf_vector_t* aliases)
{
	scf_dag_node_t*    dn_alias;
	scf_active_var_t*  status;
	scf_active_var_t*  status2;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		status   = aliases->data[i];
		dn_alias = status->alias;

//		scf_variable_t* v = dn_alias->var;
//		scf_logw("dn_alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		if (SCF_DN_ALIAS_VAR == status->alias_type) {
			if (scf_dn_through_bb(dn_alias)) {
				ret = scf_vector_add_unique(bb->entry_dn_aliases, dn_alias);
				if (ret < 0)
					return ret;
			}
		}

		status2 = calloc(1, sizeof(scf_active_var_t));
		if (!status2)
			return -ENOMEM;

		if (status->alias_indexes) {
			status2->alias_indexes = scf_vector_clone(status->alias_indexes);

			if (!status2->alias_indexes) {
				scf_active_var_free(status2);
				return -ENOMEM;
			}
		}

		ret = scf_vector_add(bb->dn_pointer_aliases, status2);
		if (ret < 0) {
			scf_active_var_free(status2);
			return ret;
		}

		status2->dag_node     = dn_pointer;
		status2->dereference  = dn_dereference;

		status2->alias        = status->alias;
		status2->alias_type   = status->alias_type;
	}
	return 0;
}

int _bb_copy_aliases2(scf_basic_block_t* bb, scf_vector_t* aliases)
{
	scf_dag_node_t*    dn_alias;
	scf_active_var_t*  status;
	scf_active_var_t*  status2;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		status   = aliases->data[i];
		dn_alias = status->alias;

		scf_variable_t* v = status->dag_node->var;
		scf_loge("dn_pointer: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		if (SCF_DN_ALIAS_VAR == status->alias_type) {
			if (scf_dn_through_bb(dn_alias)) {
				ret = scf_vector_add_unique(bb->entry_dn_aliases, dn_alias);
				if (ret < 0)
					return ret;
			}
		}

		status2 = scf_active_var_clone(status);
		if (!status2)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_pointer_aliases, status2);
		if (ret < 0) {
			scf_active_var_free(status2);
			return ret;
		}


		status2 = scf_active_var_clone(status);
		if (!status2)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_status_initeds, status2);
		if (ret < 0) {
			scf_active_var_free(status2);
			return ret;
		}
	}
	return 0;
}

static int _pointer_alias_ds(scf_vector_t* aliases, scf_active_var_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* ds;
	scf_active_var_t* ds2;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	for (l2 = &c->list; l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_prev(l2)) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);

		if (!c2->dn_status_initeds)
			continue;
#if 0
		int i;
		for (i = 0; i < c2->dn_status_initeds->size; i++) {
			ds3 = c2->dn_status_initeds->data[i];
			scf_active_var_print(ds3);
		}
#endif
		ds2 = scf_vector_find_cmp(c2->dn_status_initeds, ds_pointer, scf_dn_status_cmp_same_dn_indexes);
		if (!ds2)
			continue;

		ds = calloc(1, sizeof(scf_active_var_t));
		if (!ds)
			return -ENOMEM;

		int ret = scf_active_var_copy_alias(ds, ds2);
		if (ret < 0) {
			scf_active_var_free(ds);
			return ret;
		}

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_active_var_free(ds);
			return ret;
		}
		return 0;
	}

	return _bb_pointer_aliases(aliases, bb_list_head, bb, ds_pointer);
}

#define SCF_COPY_ALIAS_TO_POINTER(dst, src) \
			do { \
				dst = calloc(1, sizeof(scf_active_var_t)); \
				if (!dst) \
					return -ENOMEM; \
				ret = scf_active_var_copy_alias(dst, src); \
				if (ret < 0) \
					return ret; \
				dst->dag_node      = src->alias; \
				dst->dn_indexes    = src->alias_indexes; \
				dst->alias         = NULL; \
				dst->alias_indexes = NULL; \
			} while (0)

int _dn_status_alias_dereference(scf_vector_t* aliases, scf_active_var_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* ds_pointer2 = NULL;
	scf_active_var_t* status      = NULL;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	int count;
	int ret;
	int i;

	assert(ds_pointer);
	assert(ds_pointer->dag_node);

	scf_loge("dn_pointer: \n");
	scf_active_var_print(ds_pointer);

	if (SCF_OP_DEREFERENCE == ds_pointer->dag_node->type) {
		count = 0;

		for (i = 0; i < bb->dn_pointer_aliases->size; i++) {
			status    = bb->dn_pointer_aliases->data[i];

			if (ds_pointer->dag_node != status->dereference)
				continue;

			SCF_COPY_ALIAS_TO_POINTER(ds_pointer2, status);

			ret = _dn_status_alias_dereference(aliases, ds_pointer2, c, bb, bb_list_head);
			scf_active_var_free(ds_pointer2);
			ds_pointer2 = NULL;

			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			++count;
		}
		if (count > 0)
			return 0;

		assert(ds_pointer->dag_node->childs && 1 == ds_pointer->dag_node->childs->size);

		scf_vector_t* aliases2 = scf_vector_alloc();
		if (!aliases2)
			return -ENOMEM;

		ds_pointer->dag_node = ds_pointer->dag_node->childs->data[0];
		if (1) {
			// array index, pointer
		}

		ret = _dn_status_alias_dereference(aliases2, ds_pointer, c, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			scf_vector_free(aliases2);
			return ret;
		}

		for (i = 0; i < aliases2->size; i++) {
			status    = aliases2->data[i];

			SCF_COPY_ALIAS_TO_POINTER(ds_pointer2, status);

			ret = _dn_status_alias_dereference(aliases, ds_pointer2, c, bb, bb_list_head);
			scf_active_var_free(ds_pointer2);
			ds_pointer2 = NULL;

			if (ret < 0) {
				scf_loge("\n");
				scf_vector_free(aliases2);
				return ret;
			}
		}

		scf_vector_free(aliases2);
		return 0;
	}

	scf_variable_t* v = ds_pointer->dag_node->var;

	if (!scf_type_is_var(ds_pointer->dag_node->type)) {

		scf_active_var_t* ds       = NULL;
		scf_active_var_t* status2  = NULL;
		scf_vector_t*     aliases2;

		if (!c->dn_status_initeds) {
			c->dn_status_initeds = scf_vector_alloc();

			if (!c->dn_status_initeds)
				return -ENOMEM;
		}

		SCF_DN_STATUS_GET(status,  c ->dn_status_initeds, ds_pointer->dag_node);
		SCF_DN_STATUS_GET(status2, bb->dn_status_initeds, ds_pointer->dag_node);

		aliases2 = scf_vector_alloc();
		if (!aliases2)
			return -ENOMEM;

		ret = scf_pointer_alias(aliases2, ds_pointer->dag_node, c, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			scf_vector_free(aliases2);
			return -1;
		}
		assert(aliases2->size > 0);

		ds = aliases2->data[0];
		ret = scf_active_var_copy_alias(status, ds);
		if (ret < 0) {
			scf_loge("\n");
			scf_vector_free(aliases2);
			return -1;
		}

		ret = scf_active_var_copy_alias(status2, ds);
		if (ret < 0) {
			scf_loge("\n");
			scf_vector_free(aliases2);
			return -1;
		}
		scf_vector_free(aliases2);

		return scf_vector_add_unique(aliases, status);
	}

	if (ds_pointer->dag_node->var->arg_flag) {
		scf_variable_t* v = ds_pointer->dag_node->var;
		scf_logw("arg: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		return 0;
	}

	for (l2 = &c->list; l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_prev(l2)) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);

		if (!c2->dn_status_initeds)
			continue;
#if 0
		int i;
		for (i = 0; i < c2->dn_status_initeds->size; i++) {
			status = c2->dn_status_initeds->data[i];
			if (status->alias) {
				scf_variable_t* v2 = status->dag_node->var;
				v = status->alias->var;
				scf_loge("dn2: v_%d_%d/%s, status->alias: v_%d_%d/%s\n",
						v2->w->line, v2->w->pos, v2->w->text->data,
						v->w->line, v->w->pos, v->w->text->data);
			}
		}
#endif
		status = scf_vector_find_cmp(c2->dn_status_initeds, ds_pointer, scf_dn_status_cmp_like_dn_indexes);
		if (status && status->alias) {
			return scf_vector_add_unique(aliases, status);
		}
	}

	ret = _bb_pointer_aliases(aliases, bb_list_head, bb, ds_pointer);
	return ret;
}

int __alias_dereference(scf_vector_t* aliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* ds = calloc(1, sizeof(scf_active_var_t));
	if (!ds)
		return -ENOMEM;
	ds->dag_node = dn_pointer;

	int ret = _dn_status_alias_dereference(aliases, ds, c, bb, bb_list_head);

	scf_active_var_free(ds);
	return ret;
}

static int _pointer_alias_array_index2(scf_active_var_t* ds, scf_dag_node_t* dn)
{
	scf_dag_node_t* dn_base;
	scf_dag_node_t* dn_index;
	scf_variable_t* v;

	dn_base = dn;

	v = dn_base->var;
	scf_logw("dn_base: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

	while (SCF_OP_ARRAY_INDEX == dn_base->type
			|| SCF_OP_POINTER == dn_base->type) {

		dn_index = dn_base->childs->data[1];

		int ret = scf_dn_status_alias_index(ds, dn_index, dn_base->type);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		dn_base  = dn_base->childs->data[0];
		scf_logw("\n");
	}

	if (scf_variable_nb_pointers(dn_base->var) > 0) {

		ds->alias      = dn_base;
		ds->alias_type = SCF_DN_ALIAS_ARRAY;

	} else if (dn_base->var->type >= SCF_STRUCT) {

		ds->alias      = dn_base;
		ds->alias_type = SCF_DN_ALIAS_MEMBER;
	} else {
		scf_loge("\n");
		return -1;
	}

	scf_logw("\n");
	scf_active_var_print(ds);
	return 0;
}

static int _pointer_alias_address_of(scf_vector_t* aliases, scf_dag_node_t* dn_alias)
{
	scf_active_var_t* ds;
	scf_dag_node_t*   dn_child;
	scf_variable_t*   v;

	int ret;
	int type;

	ds = calloc(1, sizeof(scf_active_var_t));
	if (!ds)
		return -ENOMEM;

	dn_child = dn_alias->childs->data[0];

	switch (dn_alias->childs->size) {
		case 1:
			assert(scf_type_is_var(dn_child->type));
			ds->alias      = dn_child;
			ds->alias_type = SCF_DN_ALIAS_VAR;

			v = ds->alias->var;
			scf_logw("alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
			break;
		case 2:
		case 3:
			ds->alias_indexes = scf_vector_alloc();
			if (!ds->alias_indexes) {
				scf_active_var_free(ds);
				return -ENOMEM;
			}

			if (2 == dn_alias->childs->size)
				type = SCF_OP_POINTER;
			else
				type = SCF_OP_ARRAY_INDEX;

			ret = scf_dn_status_alias_index(ds, dn_alias->childs->data[1], type);
			if (ret < 0) {
				scf_loge("\n");
				scf_active_var_free(ds);
				return ret;
			}

			ret = _pointer_alias_array_index2(ds, dn_child);
			if (ret < 0) {
				scf_loge("\n");
				scf_active_var_free(ds);
				return ret;
			}
			break;
		default:
			scf_loge("\n");
			scf_active_var_free(ds);
			return -1;
			break;
	};

	ret = scf_vector_add(aliases, ds);
	if (ret < 0) {
		scf_active_var_free(ds);
		return ret;
	}
	return 0;
}

static int _pointer_alias_var(scf_vector_t* aliases, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_variable_t*   v = dn_alias->var;
	scf_active_var_t* ds;
	scf_active_var_t* ds2;
	scf_dn_index_t*   di;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	int ret;
	int i;

	scf_loge("alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

	if (dn_alias->var->nb_dimentions > 0) {

		ds = calloc(1, sizeof(scf_active_var_t));
		if (!ds)
			return -ENOMEM;

		ds->alias_indexes = scf_vector_alloc();
		if (!ds->alias_indexes) {
			scf_active_var_free(ds);
			return -ENOMEM;
		}

		for (i = dn_alias->var->nb_dimentions - 1; i >= 0; i--) {

			di = scf_dn_index_alloc();
			if (!di) {
				scf_active_var_free(ds);
				return -ENOMEM;
			}
			di->index = 0;

			ret = scf_vector_add(ds->alias_indexes, di);
			if (ret < 0) {
				scf_dn_index_free(di);
				scf_active_var_free(ds);
				return ret;
			}
		}

		ds->alias      = dn_alias;
		ds->alias_type = SCF_DN_ALIAS_ARRAY;

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_active_var_free(ds);
			return ret;
		}
		return 0;
	} else if (dn_alias->var->nb_pointers > 0) {

		scf_active_var_t* ds_pointer = calloc(1, sizeof(scf_active_var_t));
		if (!ds_pointer)
			return -ENOMEM;
		ds_pointer->dag_node = dn_alias;

		ret = _pointer_alias_ds(aliases, ds_pointer, c, bb, bb_list_head);
		scf_active_var_free(ds_pointer);
		return ret;
	}

	scf_loge("\n");
	return -1;
}

static int _pointer_alias_array_index(scf_vector_t* aliases, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* ds;
	scf_active_var_t* ds2;
	scf_active_var_t* ds3;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	ds2 = calloc(1, sizeof(scf_active_var_t));
	if (!ds2)
		return -ENOMEM;

	ds2->alias_indexes = scf_vector_alloc();
	if (!ds2->alias_indexes) {
		scf_active_var_free(ds2);
		return -ENOMEM;
	}

	int ret = _pointer_alias_array_index2(ds2, dn);
	if (ret < 0) {
		scf_loge("\n");
		scf_active_var_free(ds2);
		return -1;
	}

	ds2->dag_node      = ds2->alias;
	ds2->dn_indexes    = ds2->alias_indexes;
	ds2->alias         = NULL;
	ds2->alias_indexes = NULL;

	scf_active_var_print(ds2);

	ret = _pointer_alias_ds(aliases, ds2, c, bb, bb_list_head);
	scf_active_var_free(ds2);
	if (ret < 0) {
		scf_loge("\n");
	}
	return ret;
}

static int _pointer_alias_call(scf_vector_t* aliases, scf_dag_node_t* dn_alias)
{
	scf_active_var_t* ds = calloc(1, sizeof(scf_active_var_t));
	if (!ds)
		return -ENOMEM;

	int ret = scf_vector_add(aliases, ds);
	if (ret < 0) {
		scf_active_var_free(ds);
		return ret;
	}

	ds->alias      = dn_alias;
	ds->alias_type = SCF_DN_ALIAS_ALLOC;
	return 0;
}

int scf_pointer_alias(scf_vector_t* aliases, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dag_node_t* dn_child = NULL;
	scf_variable_t* v;

	while (SCF_OP_TYPE_CAST == dn_alias->type) {

		assert(dn_alias->childs && 1 == dn_alias->childs->size);

		dn_alias = dn_alias->childs->data[0];
	}

	if (scf_type_is_var(dn_alias->type))
		return _pointer_alias_var(aliases, dn_alias, c, bb, bb_list_head);

	switch (dn_alias->type) {

		case SCF_OP_ADDRESS_OF:
			return _pointer_alias_address_of(aliases, dn_alias);
			break;

		case SCF_OP_ARRAY_INDEX:
		case SCF_OP_POINTER:
			return _pointer_alias_array_index(aliases, dn_alias, c, bb, bb_list_head);
			break;

		case SCF_OP_CALL:
		case SCF_OP_3AC_CALL_EXTERN:
			return _pointer_alias_call(aliases, dn_alias);
			break;

		case SCF_OP_ADD:
			scf_loge("\n");
			return -1;
			break;
		default:
			if (dn_alias->var && dn_alias->var->w) {
				v = dn_alias->var;
				scf_loge("type: %d, v_%d_%d/%s\n", dn_alias->type, v->w->line, v->w->pos, v->w->text->data);
			} else
				scf_loge("type: %d, v_%#lx\n", dn_alias->type, 0xffff & (uintptr_t)dn_alias->var);
			return -1;
			break;
	}
	return 0;
}

