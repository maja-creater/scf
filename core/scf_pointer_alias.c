#include"scf_optimizer.h"
#include"scf_pointer_alias.h"

static int __bb_dfs_initeds(scf_basic_block_t* root, scf_dn_status_t* ds, scf_vector_t* initeds)
{
	scf_basic_block_t* bb;
	scf_dn_status_t*   ds2;

	int i;
	int j;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	int like = scf_dn_status_is_like(ds);

	for (i = 0; i < root->prevs->size; ++i) {
		bb =        root->prevs->data[i];

		if (bb->visited_flag)
			continue;

		for (j = 0; j < bb->dn_status_initeds->size; j++) {
			ds2       = bb->dn_status_initeds->data[j];

			if (like) {
				if (0 == scf_dn_status_cmp_like_dn_indexes(ds, ds2))
					break;
			} else {
				if (0 == scf_dn_status_cmp_same_dn_indexes(ds, ds2))
					break;
			}
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
		bb =        root->nexts->data[i];

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

static int _bb_dfs_initeds(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dn_status_t* ds, scf_vector_t* initeds)
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
		bb2       = initeds->data[i];

		bb2->visited_flag = 1;
	}

	l = scf_list_head(bb_list_head);
	bb2 = scf_list_data(l, scf_basic_block_t, list);

	return __bb_dfs_check_initeds(bb2, bb);
}

static int _bb_pointer_initeds(scf_vector_t* initeds, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dn_status_t* ds)
{
	scf_dag_node_t* dn;
	scf_variable_t* v;

	int ret = _bb_dfs_initeds(bb_list_head, bb, ds, initeds);
	if (ret < 0)
		return ret;

	if (0 == initeds->size) {
		dn = ds->dag_node;
		v  = dn->var;

		if (scf_variable_const(v) || scf_variable_const_string(v))
			return 0;

		if (v->arg_flag)
			return 0;

		if (v->global_flag) {
			scf_loge("bb: %p, index: %d, global pointer v_%d_%d/%s is not inited\n", bb, bb->index, v->w->line, v->w->pos, v->w->text->data);
			return 0;
		}

		if (v->nb_dimentions > 0 && !ds->dn_indexes)
			return 0;

		if (dn->node->split_flag) {
			scf_loge("dn->node->split_parent: %d, %p\n", dn->node->split_parent->type, dn->node->split_parent);
			assert(dn->node->split_parent->type == SCF_OP_CALL
					|| dn->node->split_parent->type == SCF_OP_CREATE);

			return 0;
		}

		if (ds->dn_indexes)
			return 0;

		scf_loge("bb: %p, index: %d, pointer v_%d_%d/%s is not inited\n", bb, bb->index, v->w->line, v->w->pos, v->w->text->data);

		return SCF_POINTER_NOT_INIT;
	}

	if (ds->dn_indexes && ds->dn_indexes->size > 0) {

		scf_dn_status_t* base;
		scf_vector_t*    tmp;

		base = scf_dn_status_alloc(ds->dag_node);
		if (!base)
			return -ENOMEM;

		tmp = scf_vector_alloc();
		if (!tmp) {
			scf_dn_status_free(base);
			return -ENOMEM;
		}

		ret = _bb_pointer_initeds(tmp, bb_list_head, bb, base);

		scf_vector_free(tmp);
		scf_dn_status_free(base);

		return ret;
	}

	scf_loge("initeds->size: %d\n", initeds->size);
	int i;
	for (i = 0; i < initeds->size; i++) {
		scf_basic_block_t* bb2 = initeds->data[i];

		scf_loge("bb2: %p, %d\n", bb2, bb2->index);
	}

	ret = _bb_dfs_check_initeds(bb_list_head, bb, initeds);
	if (ret < 0) {
		dn = ds->dag_node;
		v  = dn->var;

		if (!v->arg_flag && !v->global_flag) {
			scf_loge("in bb: %p, index: %d, pointer v_%d_%d/%s may not be inited\n",
					bb, bb->index, v->w->line, v->w->pos, v->w->text->data);

			return SCF_POINTER_NOT_INIT;
		}
		return 0;
	}
	return ret;
}

static int _bb_pointer_initeds_leak(scf_vector_t* initeds, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dn_status_t* ds)
{
	scf_dag_node_t* dn;
	scf_variable_t* v;

	int ret = _bb_dfs_initeds(bb_list_head, bb, ds, initeds);
	if (ret < 0)
		return ret;

	if (0 == initeds->size) {
		dn = ds->dag_node;
		v  = dn->var;

		if (scf_variable_const(v) || scf_variable_const_string(v))
			return 0;

		if (v->arg_flag)
			return 0;

		if (v->global_flag) {
			scf_loge("bb: %p, index: %d, global pointer v_%d_%d/%s is not inited\n", bb, bb->index, v->w->line, v->w->pos, v->w->text->data);
			return 0;
		}

		if (v->nb_dimentions > 0 && !ds->dn_indexes)
			return 0;

		if (dn->node->split_flag) {
			scf_loge("dn->node->split_parent: %d, %p\n", dn->node->split_parent->type, dn->node->split_parent);
			assert(dn->node->split_parent->type == SCF_OP_CALL
					|| dn->node->split_parent->type == SCF_OP_CREATE);

			return 0;
		}

		if (ds->dn_indexes)
			return 0;

		scf_loge("bb: %p, index: %d, pointer v_%d_%d/%s is not inited\n", bb, bb->index, v->w->line, v->w->pos, v->w->text->data);

		return SCF_POINTER_NOT_INIT;
	}

	if (ds->dn_indexes && ds->dn_indexes->size > 0) {

		scf_dn_status_t* base;
		scf_vector_t*    tmp;

		base = scf_dn_status_alloc(ds->dag_node);
		if (!base)
			return -ENOMEM;

		tmp = scf_vector_alloc();
		if (!tmp) {
			scf_dn_status_free(base);
			return -ENOMEM;
		}

		ret = _bb_pointer_initeds_leak(tmp, bb_list_head, bb, base);

		scf_vector_free(tmp);
		scf_dn_status_free(base);

		return ret;
	}

	return 0;
}

static int _bb_pointer_aliases(scf_vector_t* aliases, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dn_status_t* ds_pointer)
{
	scf_vector_t*      initeds;
	scf_basic_block_t* bb2;
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;

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
			ds =        bb2->dn_status_initeds->data[j];

			if (scf_dn_status_cmp_like_dn_indexes(ds, ds_pointer))
				continue;

			if (scf_vector_find_cmp(aliases, ds, scf_dn_status_cmp_alias))
				continue;

			ds2 = scf_dn_status_clone(ds);
			if (!ds2) {
				scf_vector_free(initeds);
				return -ENOMEM;
			}

			ret = scf_vector_add(aliases, ds2);
			if (ret < 0) {
				scf_dn_status_free(ds2);
				scf_vector_free(initeds);
				return ret;
			}
		}
	}

	return 0;
}

static int _bb_pointer_aliases_leak(scf_vector_t* aliases, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dn_status_t* ds_pointer)
{
	scf_vector_t*      initeds;
	scf_basic_block_t* bb2;
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;

	int ret;
	int i;
	int j;

	initeds = scf_vector_alloc();
	if (!initeds)
		return -ENOMEM;

	ret = _bb_pointer_initeds_leak(initeds, bb_list_head, bb, ds_pointer);
	if (ret < 0) {
		scf_vector_free(initeds);
		return ret;
	}

	for (i = 0; i < initeds->size; i++) {
		bb2       = initeds->data[i];

		for (j = 0; j < bb2->dn_status_initeds->size; j++) {
			ds =        bb2->dn_status_initeds->data[j];

			if (scf_dn_status_cmp_like_dn_indexes(ds, ds_pointer))
				continue;

			if (scf_vector_find_cmp(aliases, ds, scf_dn_status_cmp_alias))
				continue;

			ds2 = scf_dn_status_clone(ds);
			if (!ds2) {
				scf_vector_free(initeds);
				return -ENOMEM;
			}

			ret = scf_vector_add(aliases, ds2);
			if (ret < 0) {
				scf_dn_status_free(ds2);
				scf_vector_free(initeds);
				return ret;
			}
		}
	}

	return 0;
}

int scf_pointer_alias_ds_leak(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dn_status_t* ds3;
	scf_3ac_code_t*  c2;
	scf_list_t*      l2;

	for (l2 = &c->list; l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_prev(l2)) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);

		if (!c2->dn_status_initeds)
			continue;

		ds2 = scf_vector_find_cmp(c2->dn_status_initeds, ds_pointer, scf_dn_status_cmp_same_dn_indexes);
		if (!ds2)
			continue;

		ds = scf_dn_status_null(); 
		if (!ds)
			return -ENOMEM;

		int ret = scf_dn_status_copy_alias(ds, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}

		return 0;
	}

	return _bb_pointer_aliases_leak(aliases, bb_list_head, bb, ds_pointer);
}

int _bb_copy_aliases(scf_basic_block_t* bb, scf_dag_node_t* dn_pointer, scf_dag_node_t* dn_dereference, scf_vector_t* aliases)
{
	scf_dag_node_t*   dn_alias;
	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		ds        = aliases->data[i];

		dn_alias  = ds->alias;

//		scf_variable_t* v = dn_alias->var;
//		scf_logw("dn_alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		if (SCF_DN_ALIAS_VAR == ds->alias_type) {

			if (scf_dn_through_bb(dn_alias)) {

				ret = scf_vector_add_unique(bb->entry_dn_aliases, dn_alias);
				if (ret < 0)
					return ret;
			}
		}

		ds2 = scf_dn_status_null();
		if (!ds2)
			return -ENOMEM;

		if (scf_dn_status_copy_alias(ds2, ds) < 0) {
			scf_dn_status_free(ds2);
			return -ENOMEM;
		}

		ret = scf_vector_add(bb->dn_pointer_aliases, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			return ret;
		}

		ds2->dag_node     = dn_pointer;
		ds2->dereference  = dn_dereference;
	}

	return 0;
}

int _bb_copy_aliases2(scf_basic_block_t* bb, scf_vector_t* aliases)
{
	scf_dag_node_t*   dn_alias;
	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		ds        = aliases->data[i];
		dn_alias  = ds->alias;

		scf_variable_t* v = ds->dag_node->var;
		scf_logd("dn_pointer: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		if (SCF_DN_ALIAS_VAR == ds->alias_type) {

			if (scf_dn_through_bb(dn_alias)) {

				ret = scf_vector_add_unique(bb->entry_dn_aliases, dn_alias);
				if (ret < 0)
					return ret;
			}
		}

		ds2 = scf_dn_status_clone(ds);
		if (!ds2)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_pointer_aliases, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			return ret;
		}

		ds2 = scf_dn_status_clone(ds);
		if (!ds2)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_status_initeds, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			return ret;
		}
	}
	return 0;
}

int scf_pointer_alias_ds(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dn_status_t* ds3;
	scf_3ac_code_t*  c2;
	scf_list_t*      l2;

	for (l2 = &c->list; l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_prev(l2)) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);

		if (!c2->dn_status_initeds)
			continue;

		ds2 = scf_vector_find_cmp(c2->dn_status_initeds, ds_pointer, scf_dn_status_cmp_same_dn_indexes);
		if (!ds2)
			continue;

		ds = scf_dn_status_null(); 
		if (!ds)
			return -ENOMEM;

		int ret = scf_dn_status_copy_alias(ds, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}

		return 0;
	}

	return _bb_pointer_aliases(aliases, bb_list_head, bb, ds_pointer);
}

static int _bb_op_aliases(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t*  ds  = NULL;
	scf_dn_status_t* _ds  = NULL;
	scf_dn_status_t* _ds2 = NULL;
	scf_vector_t*    aliases2;

	if (!c->dn_status_initeds) {
		c->dn_status_initeds = scf_vector_alloc();

		if (!c->dn_status_initeds)
			return -ENOMEM;
	}

	SCF_DN_STATUS_GET(_ds,  c ->dn_status_initeds, ds_pointer->dag_node);
	SCF_DN_STATUS_GET(_ds2, bb->dn_status_initeds, ds_pointer->dag_node);

	aliases2 = scf_vector_alloc();
	if (!aliases2)
		return -ENOMEM;

	int ret = scf_pointer_alias(aliases2, ds_pointer->dag_node, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	assert(aliases2->size > 0);

	ds = aliases2->data[0];
	ret = scf_dn_status_copy_alias(_ds, ds);
	if (ret < 0)
		goto error;

	ret = scf_dn_status_copy_alias(_ds2, ds);
	if (ret < 0)
		goto error;

	if (!scf_vector_find(aliases, _ds)) {

		ret = scf_vector_add(aliases, _ds);
		if (ret < 0)
			goto error;

		scf_dn_status_ref(_ds);
	}

	ret = 0;
error:
	scf_vector_clear(aliases2, ( void (*)(void*) )scf_dn_status_free);
	scf_vector_free(aliases2);
	return ret;
}

int _dn_status_alias_dereference(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);

static int _bb_dereference_aliases(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_pointer2 = NULL;
	scf_dn_status_t* ds = NULL;
	scf_3ac_code_t*  c2;

	int count = 0;
	int ret;
	int i;

	for (i = 0; i < bb->dn_pointer_aliases->size; i++) {
		ds =        bb->dn_pointer_aliases->data[i];

		if (ds_pointer->dag_node != ds->dereference)
			continue;

#define SCF_DS_POINTER_FROM_ALIAS(dst, src) \
		do { \
			dst = scf_dn_status_null(); \
			if (!dst) \
			return -ENOMEM; \
			\
			ret = scf_dn_status_copy_alias(dst, src); \
			if (ret < 0) { \
				scf_dn_status_free(dst); \
				return ret; \
			} \
			\
			dst->dag_node      = dst->alias; \
			dst->dn_indexes    = dst->alias_indexes; \
			dst->alias         = NULL; \
			dst->alias_indexes = NULL; \
		} while (0)

		SCF_DS_POINTER_FROM_ALIAS(ds_pointer2, ds);

		ret = _dn_status_alias_dereference(aliases, ds_pointer2, c, bb, bb_list_head);

		scf_dn_status_free(ds_pointer2);
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

	ret = _dn_status_alias_dereference(aliases2, ds_pointer, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	for (i = 0; i < aliases2->size; i++) {
		ds =        aliases2->data[i];

		SCF_DS_POINTER_FROM_ALIAS(ds_pointer2, ds);

		ret = _dn_status_alias_dereference(aliases, ds_pointer2, c, bb, bb_list_head);

		scf_dn_status_free(ds_pointer2);
		ds_pointer2 = NULL;

		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}
	}

	ret = 0;
error:
	scf_vector_clear(aliases2, ( void (*)(void*) )scf_dn_status_free);
	scf_vector_free(aliases2);
	return ret;
}

int _dn_status_alias_dereference(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dag_node_t*  dn = ds_pointer->dag_node;
	scf_dn_status_t* ds = NULL;
	scf_3ac_code_t*  c2;
	scf_list_t*      l2;

	int count;
	int ret;
	int i;

	assert(ds_pointer);
	assert(ds_pointer->dag_node);

//	scf_logw("dn_pointer: \n");
//	scf_dn_status_print(ds_pointer);

	if (SCF_OP_DEREFERENCE == dn->type)
		return _bb_dereference_aliases(aliases, ds_pointer, c, bb, bb_list_head);

	if (!scf_type_is_var(dn->type)
			&& SCF_OP_INC != dn->type && SCF_OP_INC_POST != dn->type
			&& SCF_OP_DEC != dn->type && SCF_OP_DEC_POST != dn->type)

		return _bb_op_aliases(aliases, ds_pointer, c, bb, bb_list_head);


	scf_variable_t* v = dn->var;

	if (v->arg_flag) {
		scf_logd("arg: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		return 0;
	} else if (v->global_flag) {
		scf_logd("global: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		return 0;
	} else if (SCF_FUNCTION_PTR == v->type) {
		scf_logd("funcptr: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		return 0;
	}

	for (l2 = &c->list; l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_prev(l2)) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);

		if (!c2->dn_status_initeds)
			continue;

		ds = scf_vector_find_cmp(c2->dn_status_initeds, ds_pointer, scf_dn_status_cmp_like_dn_indexes);
		if (ds && ds->alias) {

			if (scf_vector_find(aliases, ds))
				return 0;

			if (scf_vector_add(aliases, ds) < 0)
				return -ENOMEM;

			scf_dn_status_ref(ds);
			return 0; 
		}
	}

	return _bb_pointer_aliases(aliases, bb_list_head, bb, ds_pointer);
}

int __alias_dereference(scf_vector_t* aliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds = scf_dn_status_null(); 
	if (!ds)
		return -ENOMEM;
	ds->dag_node = dn_pointer;

	int ret = _dn_status_alias_dereference(aliases, ds, c, bb, bb_list_head);

	scf_dn_status_free(ds);

	return ret;
}

static int _pointer_alias_array_index2(scf_dn_status_t* ds, scf_dag_node_t* dn)
{
	scf_dag_node_t* dn_base;
	scf_dag_node_t* dn_index;
	scf_dn_index_t* di;
	scf_variable_t* v;

	dn_base = dn;

	v = dn_base->var;

	while (SCF_OP_ARRAY_INDEX == dn_base->type
			|| SCF_OP_POINTER == dn_base->type) {

		dn_index = dn_base->childs->data[1];

		int ret = scf_dn_status_alias_index(ds, dn_index, dn_base->type);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (SCF_OP_ARRAY_INDEX == dn_base->type) {

			assert(dn_base->childs->size >= 3);

			di           = ds->alias_indexes->data[ds->alias_indexes->size - 1];
			di->dn_scale = dn_base->childs->data[2];

			assert(di->dn_scale);
		}

		dn_base  = dn_base->childs->data[0];
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

	return 0;
}

static int _pointer_alias_address_of(scf_vector_t* aliases, scf_dag_node_t* dn_alias)
{
	scf_dn_status_t*  ds;
	scf_dag_node_t*   dn_child;
	scf_variable_t*   v;

	int ret;
	int type;

	ds = scf_dn_status_null();
	if (!ds)
		return -ENOMEM;

	dn_child = dn_alias->childs->data[0];

	switch (dn_alias->childs->size) {
		case 1:
			assert(scf_type_is_var(dn_child->type));
			ds->alias      = dn_child;
			ds->alias_type = SCF_DN_ALIAS_VAR;

			v = ds->alias->var;
			scf_logd("alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
			break;
		case 2:
		case 3:
			ds->alias_indexes = scf_vector_alloc();
			if (!ds->alias_indexes) {
				scf_dn_status_free(ds);
				return -ENOMEM;
			}

			if (2 == dn_alias->childs->size)
				type = SCF_OP_POINTER;
			else
				type = SCF_OP_ARRAY_INDEX;

			ret = scf_dn_status_alias_index(ds, dn_alias->childs->data[1], type);
			if (ret < 0) {
				scf_loge("\n");
				scf_dn_status_free(ds);
				return ret;
			}

			if (SCF_OP_ARRAY_INDEX == dn_alias->type) {

				scf_dn_index_t* di;

				assert(dn_alias->childs->size >= 3);

				di           = ds->alias_indexes->data[ds->alias_indexes->size - 1];
				di->dn_scale = dn_alias->childs->data[2];

				assert(di->dn_scale);
			}

			ret = _pointer_alias_array_index2(ds, dn_child);
			if (ret < 0) {
				scf_loge("\n");
				scf_dn_status_free(ds);
				return ret;
			}
			break;
		default:
			scf_loge("\n");
			scf_dn_status_free(ds);
			return -1;
			break;
	};

	ret = scf_vector_add(aliases, ds);
	if (ret < 0) {
		scf_dn_status_free(ds);
		return ret;
	}
	return 0;
}

static int _ds_alias_array_indexes(scf_dn_status_t* ds, scf_dag_node_t* dn_alias, int nb_dimentions)
{
	scf_dn_index_t*   di;
	int i;

	assert(ds);
	assert(dn_alias);
	assert(nb_dimentions >= 1);

	ds->alias_indexes = scf_vector_alloc();
	if (!ds->alias_indexes)
		return -ENOMEM;

	for (i = nb_dimentions - 1; i >= 0; i--) {

		di = scf_dn_index_alloc();
		if (!di)
			return -ENOMEM;
		di->index = 0;

		int ret = scf_vector_add(ds->alias_indexes, di);
		if (ret < 0) {
			scf_dn_index_free(di);
			return ret;
		}
	}

	ds->alias      = dn_alias;
	ds->alias_type = SCF_DN_ALIAS_ARRAY;
	return 0;
}

static int _pointer_alias_var(scf_vector_t* aliases, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_variable_t*   v = dn_alias->var;
	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;
	scf_dn_index_t*   di;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	int ret;
	int i;

	scf_logd("alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

	if (dn_alias->var->nb_dimentions > 0) {

		ds = scf_dn_status_null(); 
		if (!ds)
			return -ENOMEM;

		ret = _ds_alias_array_indexes(ds, dn_alias, dn_alias->var->nb_dimentions);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}
		return 0;
	} else if (dn_alias->var->nb_pointers > 0) {

		scf_dn_status_t* ds_pointer = scf_dn_status_null();
		if (!ds_pointer)
			return -ENOMEM;
		ds_pointer->dag_node = dn_alias;

		ret = scf_pointer_alias_ds(aliases, ds_pointer, c, bb, bb_list_head);
		scf_dn_status_free(ds_pointer);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (0 == aliases->size) {

			ds = scf_dn_status_null();
			if (!ds)
				return -ENOMEM;

			ds->alias      = dn_alias;
			ds->alias_type = SCF_DN_ALIAS_VAR;

			ret = scf_vector_add(aliases, ds);
			if (ret < 0) {
				scf_dn_status_free(ds);
				return ret;
			}
		}

		return ret;
	} else if (sizeof(void*) == dn_alias->var->size) {

		ds = scf_dn_status_null();
		if (!ds)
			return -ENOMEM;

		ds->alias      = dn_alias;
		ds->alias_type = SCF_DN_ALIAS_VAR;

		scf_dn_status_print(ds);

		ret = scf_vector_add(aliases, ds);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}
		return ret;
	}

	scf_loge("\n");
	return -1;
}

static int _pointer_alias_array_index(scf_vector_t* aliases, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dn_status_t* ds3;
	scf_3ac_code_t*   c2;
	scf_list_t*       l2;

	ds2 = scf_dn_status_null();
	if (!ds2)
		return -ENOMEM;

	ds2->alias_indexes = scf_vector_alloc();
	if (!ds2->alias_indexes) {
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	int ret = _pointer_alias_array_index2(ds2, dn);
	if (ret < 0) {
		scf_loge("\n");
		scf_dn_status_free(ds2);
		return -1;
	}

	ds2->dag_node      = ds2->alias;
	ds2->dn_indexes    = ds2->alias_indexes;
	ds2->alias         = NULL;
	ds2->alias_indexes = NULL;

	ret = scf_pointer_alias_ds(aliases, ds2, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		scf_dn_status_free(ds2);
		return ret;
	}

	if (0 == aliases->size) {
		ds2->alias         = ds2->dag_node;
		ds2->alias_indexes = ds2->dn_indexes;
		ds2->dag_node      = NULL;
		ds2->dn_indexes    = NULL;

		ret = scf_vector_add(aliases, ds2);
		if (ret < 0) {
			scf_loge("\n");
			scf_dn_status_free(ds2);
			return ret;
		}
	} else {
		scf_dn_status_free(ds2);
	}
	return ret;
}

static int _pointer_alias_call(scf_vector_t* aliases, scf_dag_node_t* dn_alias)
{
	scf_dn_status_t* ds = scf_dn_status_null();
	if (!ds)
		return -ENOMEM;

	int ret = scf_vector_add(aliases, ds);
	if (ret < 0) {
		scf_dn_status_free(ds);
		return ret;
	}

	ds->alias      = dn_alias;
	ds->alias_type = SCF_DN_ALIAS_ALLOC;
	return 0;
}

static int _pointer_alias_dereference(scf_vector_t* aliases, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dag_node_t*  dn_child;
	scf_vector_t*    aliases2 = NULL;
	scf_vector_t*    aliases3 = NULL;

	assert(dn_alias->childs && 1 == dn_alias->childs->size);

	dn_child = dn_alias->childs->data[0];

	aliases2 = scf_vector_alloc();
	if (!aliases2) {
		scf_loge("\n");
		return -ENOMEM;
	}

	int ret = scf_pointer_alias(aliases2, dn_child, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	int i;
	for (i = 0; i < aliases2->size; i++) {
		ds =        aliases2->data[i];

		if (!ds)
			continue;

		ds2 = scf_dn_status_null();
		if (!ds2) {
			ret = -ENOMEM;
			goto error;
		}

		ret = scf_dn_status_copy_alias(ds2, ds);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			goto error;
		}

		ds2->dag_node      = ds2->alias;
		ds2->dn_indexes    = ds2->alias_indexes;
		ds2->alias         = NULL;
		ds2->alias_indexes = NULL;

		aliases3 = scf_vector_alloc();
		if (!aliases3) {
			scf_loge("\n");

			scf_dn_status_free(ds2);
			ret = -ENOMEM;
			goto error;
		}

		ret = scf_pointer_alias_ds(aliases3, ds2, c, bb, bb_list_head);

		if (ret < 0) {
			scf_dn_status_free(ds2);
			scf_vector_free(aliases3);
			goto error;
		}

		if (0 == aliases3->size) {

			ds2->alias         = ds2->dag_node;
			ds2->alias_indexes = ds2->dn_indexes;
			ds2->dag_node      = NULL;
			ds2->dn_indexes    = NULL;

			ret = scf_vector_add(aliases, ds2);
			if (ret < 0) {
				scf_dn_status_free(ds2);
				scf_vector_free(aliases3);
				goto error;
			}

			scf_dn_status_ref(ds);

		} else {
			scf_dn_status_free(ds2);
			ds2 = NULL;

			int j;
			for (j = 0; j < aliases3->size; j++) {
				ds =        aliases3->data[j];

				ret = scf_vector_add(aliases, ds);
				if (ret < 0) {
					scf_vector_free(aliases3);
					goto error;
				}

				scf_dn_status_ref(ds);
			}
		}

		scf_vector_clear(aliases3, (void (*)(void*))scf_dn_status_free);
		scf_vector_free(aliases3);
		aliases3 = NULL;
	}

error:
	scf_vector_clear(aliases2, (void (*)(void*))scf_dn_status_free);
	scf_vector_free(aliases2);
	aliases2 = NULL;
	return ret;
}

static int _pointer_alias_add(scf_vector_t* aliases, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dag_node_t*  dn0;
	scf_dag_node_t*  dn1;
	scf_dn_index_t*  di;

	assert(2 == dn->childs->size);
	dn0 = dn->childs->data[0];
	dn1 = dn->childs->data[1];

	while (SCF_OP_TYPE_CAST == dn0->type
			|| SCF_OP_EXPR  == dn0->type) {
		assert(1 == dn0->childs->size);
		dn0 = dn0->childs->data[0];
	}

	while (SCF_OP_TYPE_CAST == dn1->type
			|| SCF_OP_EXPR  == dn1->type) {
		assert(1 == dn1->childs->size);
		dn1 = dn1->childs->data[0];
	}

	di = scf_dn_index_alloc();
	if (!di) {
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	if (scf_variable_nb_pointers(dn0->var) > 0) {

		ds2 = NULL;
		int ret = scf_ds_for_dn(&ds2, dn0);
		if (ret < 0)
			return ret;

		if (scf_variable_const(dn1->var))
			di->index = dn1->var->data.i64;
		else
			di->index = -1;

	} else if (scf_variable_nb_pointers(dn1->var) > 0) {

		ds2 = NULL;
		int ret = scf_ds_for_dn(&ds2, dn1);
		if (ret < 0)
			return ret;

		if (scf_variable_const(dn0->var))
			di->index = dn0->var->data.i64;
		else
			di->index = -1;
	} else {
		scf_loge("dn0->nb_pointers: %d\n", dn0->var->nb_pointers);
		scf_loge("dn1->nb_pointers: %d\n", dn1->var->nb_pointers);

		scf_dn_status_free(ds2);
		return -1;
	}

	if (!ds2->dn_indexes) {
		ds2->dn_indexes = scf_vector_alloc();

		if (!ds2->dn_indexes) {
			scf_dn_index_free(di);
			scf_dn_status_free(ds2);
			return -ENOMEM;
		}
	}

	if (scf_vector_add(ds2->dn_indexes, di) < 0) {
		scf_dn_index_free(di);
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	int i;
	for (i = ds2->dn_indexes->size - 2; i >= 0; i--)
		ds2->dn_indexes->data[i + 1] = ds2->dn_indexes->data[i];

	ds2->dn_indexes->data[0] = di;

	int ret = scf_pointer_alias_ds(aliases, ds2, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		scf_dn_status_free(ds2);
		return ret;
	}

	if (0 == aliases->size) {
		ds2->alias         = ds2->dag_node;
		ds2->alias_indexes = ds2->dn_indexes;
		ds2->dag_node      = NULL;
		ds2->dn_indexes    = NULL;
		ds2->alias_type    = SCF_DN_ALIAS_ARRAY;

		ret = scf_vector_add(aliases, ds2);
		if (ret < 0) {
			scf_loge("\n");
			scf_dn_status_free(ds2);
			return ret;
		}
	} else
		scf_dn_status_free(ds2);

	return ret;
}

static int _pointer_alias_sub(scf_vector_t* aliases, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds;
	scf_dn_status_t* ds2;
	scf_dag_node_t*  dn0;
	scf_dag_node_t*  dn1;
	scf_dn_index_t*  di;

	assert(2 == dn->childs->size);
	dn0 = dn->childs->data[0];
	dn1 = dn->childs->data[1];

	while (SCF_OP_TYPE_CAST == dn0->type
			|| SCF_OP_EXPR  == dn0->type) {
		assert(1 == dn0->childs->size);
		dn0 = dn0->childs->data[0];
	}

	while (SCF_OP_TYPE_CAST == dn1->type
			|| SCF_OP_EXPR  == dn1->type) {
		assert(1 == dn1->childs->size);
		dn1 = dn1->childs->data[0];
	}

	ds2 = scf_dn_status_null();
	if (!ds2)
		return -ENOMEM;

	ds2->dn_indexes = scf_vector_alloc();
	if (!ds2->dn_indexes) {
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	di = scf_dn_index_alloc();
	if (!di) {
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	if (scf_vector_add(ds2->dn_indexes, di) < 0) {
		scf_dn_index_free(di);
		scf_dn_status_free(ds2);
		return -ENOMEM;
	}

	if (scf_variable_nb_pointers(dn0->var) > 0) {
		assert(scf_type_is_var(dn0->type));

		ds2->dag_node = dn0;

		if (scf_variable_const(dn1->var))
			di->index = dn1->var->data.i64;
		else
			di->index = -1;
	} else {
		scf_loge("dn0->nb_pointers: %d\n", dn0->var->nb_pointers);
		scf_loge("dn1->nb_pointers: %d\n", dn1->var->nb_pointers);

		scf_dn_status_free(ds2);
		return -1;
	}

	int ret = scf_pointer_alias_ds(aliases, ds2, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		scf_dn_status_free(ds2);
		return ret;
	}

	if (0 == aliases->size) {
		ds2->alias         = ds2->dag_node;
		ds2->alias_indexes = ds2->dn_indexes;
		ds2->dag_node      = NULL;
		ds2->dn_indexes    = NULL;
		ds2->alias_type    = SCF_DN_ALIAS_ARRAY;

		ret = scf_vector_add(aliases, ds2);
		if (ret < 0) {
			scf_loge("\n");
			scf_dn_status_free(ds2);
			return ret;
		}
	} else
		scf_dn_status_free(ds2);

	return ret;
}

static int _pointer_alias_logic(scf_vector_t* aliases, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds       = NULL;
	scf_dn_status_t* ds2      = NULL;
	scf_vector_t*    aliases2 = NULL;

	assert(2 == dn->childs->size);

	int ret = scf_ds_for_dn(&ds, dn->childs->data[0]);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = scf_ds_for_dn(&ds2, dn->childs->data[1]);
	if (ret < 0) {
		scf_loge("\n");
		scf_dn_status_free(ds);
		return ret;
	}

	if (scf_ds_is_pointer(ds) > 0) {

		ret = scf_pointer_alias_ds(aliases, ds, c, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}

		if (0 == aliases->size) {
			ds->alias         = ds->dag_node;
			ds->alias_indexes = ds->dn_indexes;
			ds->dag_node      = NULL;
			ds->dn_indexes    = NULL;
			ds->alias_type    = SCF_DN_ALIAS_VAR;

			ret = scf_vector_add(aliases, ds);
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		} else {
			scf_dn_status_free(ds);
			ds = NULL;
		}

	} else if (scf_ds_is_pointer(ds2) > 0) {

		aliases2 = scf_vector_alloc();
		if (!aliases2) {
			ret = -ENOMEM;
			goto error;
		}

		ret = scf_pointer_alias_ds(aliases2, ds2, c, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}

		if (0 == aliases2->size) {
			ds2->alias         = ds2->dag_node;
			ds2->alias_indexes = ds2->dn_indexes;
			ds2->dag_node      = NULL;
			ds2->dn_indexes    = NULL;
			ds2->alias_type    = SCF_DN_ALIAS_VAR;

			ret = scf_vector_add(aliases, ds2);
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		} else {
			scf_dn_status_free(ds2);
			ds2 = NULL;

			int i;
			for (i = 0; i < aliases2->size; i++) {
				ds2       = aliases2->data[i];

				if (scf_vector_add_unique(aliases, ds2) < 0) {
					ret = -ENOMEM;
					goto error;
				}
			}

			ds2 = NULL;
		}
	} else if (!scf_variable_const(ds->dag_node->var)) {

		ds->alias         = ds->dag_node;
		ds->alias_indexes = ds->dn_indexes;
		ds->dag_node      = NULL;
		ds->dn_indexes    = NULL;
		ds->alias_type    = SCF_DN_ALIAS_VAR;

		if (scf_vector_add_unique(aliases, ds) < 0) {
			ret = -ENOMEM;
			goto error;
		}

		ds = NULL;
	} else if (!scf_variable_const(ds2->dag_node->var)) {

		ds2->alias         = ds2->dag_node;
		ds2->alias_indexes = ds2->dn_indexes;
		ds2->dag_node      = NULL;
		ds2->dn_indexes    = NULL;
		ds2->alias_type    = SCF_DN_ALIAS_VAR;

		if (scf_vector_add_unique(aliases, ds2) < 0) {
			ret = -ENOMEM;
			goto error;
		}

		ds2 = NULL;
	} else {
		ds->alias         = ds->dag_node;
		ds->alias_indexes = ds->dn_indexes;
		ds->dag_node      = NULL;
		ds->dn_indexes    = NULL;
		ds->alias_type    = SCF_DN_ALIAS_VAR;

		if (scf_vector_add_unique(aliases, ds) < 0) {
			ret = -ENOMEM;
			goto error;
		}

		ds = NULL;
	}

	return ret;
error:
	if (aliases2)
		scf_vector_free(aliases2);

	if (ds)
		scf_dn_status_free(ds);

	if (ds2)
		scf_dn_status_free(ds2);
	return ret;
}

int scf_pointer_alias(scf_vector_t* aliases, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dag_node_t* dn_child = NULL;
	scf_variable_t* v;

	v = dn_alias->var;
	scf_logd("v: %d/%s\n", v->w->line, v->w->text->data);

	while (SCF_OP_TYPE_CAST == dn_alias->type) {

		assert(dn_alias->childs && 1 == dn_alias->childs->size);

		dn_alias = dn_alias->childs->data[0];
	}

	int ret;
	if (scf_type_is_var(dn_alias->type))
		return _pointer_alias_var(aliases, dn_alias, c, bb, bb_list_head);

	switch (dn_alias->type) {

		case SCF_OP_ADDRESS_OF:
			ret = _pointer_alias_address_of(aliases, dn_alias);
			return ret;
			break;

		case SCF_OP_ARRAY_INDEX:
		case SCF_OP_POINTER:
			ret = _pointer_alias_array_index(aliases, dn_alias, c, bb, bb_list_head);
			return ret;
			break;

		case SCF_OP_CALL:
			ret = _pointer_alias_call(aliases, dn_alias);
			return ret;
			break;

		case SCF_OP_ADD:
			ret = _pointer_alias_add(aliases, dn_alias, c, bb, bb_list_head);
			return ret;
			break;

		case SCF_OP_SUB:
			ret = _pointer_alias_sub(aliases, dn_alias, c, bb, bb_list_head);
			return ret;
			break;

		case SCF_OP_BIT_AND:
		case SCF_OP_BIT_OR:
			ret = _pointer_alias_logic(aliases, dn_alias, c, bb, bb_list_head);
			return ret;
			break;

		case SCF_OP_DEREFERENCE:
			if (dn_alias->var && dn_alias->var->w) {
				v = dn_alias->var;
				scf_logd("type: %d, v_%d_%d/%s\n", dn_alias->type, v->w->line, v->w->pos, v->w->text->data);
			} else
				scf_logd("type: %d, v_%#lx\n", dn_alias->type, 0xffff & (uintptr_t)dn_alias->var);

			ret = _pointer_alias_dereference(aliases, dn_alias, c, bb, bb_list_head);
			return ret;
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

