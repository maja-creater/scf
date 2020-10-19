#include"scf_dag.h"
#include"scf_3ac.h"

scf_dn_index_t* scf_dn_index_alloc()
{
	scf_dn_index_t* di = calloc(1, sizeof(scf_dn_index_t));
	if (!di)
		return NULL;

	di->refs = 1;
	return di;
}

scf_dn_index_t* scf_dn_index_clone(scf_dn_index_t* di)
{
	if (di)
		di->refs++;

	return di;
}

void scf_dn_index_free(scf_dn_index_t* di)
{
	if (di && 0 == --di->refs)
		free(di);
}

int scf_dn_index_same(const scf_dn_index_t* di0, const scf_dn_index_t* di1)
{
	if (di0->member != di1->member)
		return 0;

	if (di0->member)
		return 1;

	if (-1 == di0->index || -1 == di1->index)
		return 0;

	if (di0->index == di1->index)
		return 1;
	return 0;
}

int scf_dn_index_like(const scf_dn_index_t* di0, const scf_dn_index_t* di1)
{
	if (di0->member != di1->member)
		return 0;

	if (di0->member)
		return 1;

	if (-1 == di0->index || -1 == di1->index)
		return 1;

	if (di0->index == di1->index)
		return 1;
	return 0;
}

scf_dn_status_t* scf_dn_status_alloc(scf_dag_node_t* dn)
{
	if (!dn)
		return NULL;

	scf_dn_status_t* v = calloc(1, sizeof(scf_dn_status_t));
	if (!v)
		return NULL;

	v->dag_node     = dn;

	v->active       = dn->active;
	v->inited       = dn->inited;
	v->updated      = dn->updated;

	return v;
}

int scf_dn_status_copy_dn(scf_dn_status_t* dst, scf_dn_status_t* src)
{
	scf_dn_index_t*   di;
	int i;

	if (!dst || !src)
		return -1;

	dst->dag_node = src->dag_node;

	if (dst->dn_indexes) {
		scf_vector_clear(dst->dn_indexes, (void (*)(void*))scf_dn_index_free);
		scf_vector_free (dst->dn_indexes);
		dst->dn_indexes = NULL;
	}

	if (src->dn_indexes) {
		dst->dn_indexes = scf_vector_clone(src->dn_indexes);

		if (!dst->dn_indexes)
			return -ENOMEM;

		for (i = 0; i < dst->dn_indexes->size; i++) {
			di = dst->dn_indexes->data[i];
			di->refs++;
		}
	}
	return 0;
}

int scf_dn_status_copy_alias(scf_dn_status_t* dst, scf_dn_status_t* src)
{
	scf_dn_index_t*   di;
	int i;

	if (!dst || !src)
		return -1;

	dst->alias      = src->alias;
	dst->alias_type = src->alias_type;

	if (dst->alias_indexes) {
		scf_vector_clear(dst->alias_indexes, (void (*)(void*))scf_dn_index_free);
		scf_vector_free (dst->alias_indexes);
		dst->alias_indexes = NULL;
	}

	if (src->alias_indexes) {
		dst->alias_indexes = scf_vector_clone(src->alias_indexes);

		if (!dst->alias_indexes)
			return -ENOMEM;

		for (i = 0; i < dst->alias_indexes->size; i++) {
			di = dst->alias_indexes->data[i];
			di->refs++;
		}
	}
	return 0;
}

scf_dn_status_t* scf_dn_status_clone(scf_dn_status_t* v)
{
	scf_dn_status_t* v2;
	scf_dn_index_t*   di;
	int i;

	if (!v)
		return NULL;

	v2 = calloc(1, sizeof(scf_dn_status_t));
	if (!v2)
		return NULL;

	if (v ->dn_indexes) {
		v2->dn_indexes = scf_vector_clone(v->dn_indexes);

		if (!v2->dn_indexes) {
			scf_dn_status_free(v2);
			return NULL;
		}

		for (i = 0; i < v2->dn_indexes->size; i++) {
			di = v2->dn_indexes->data[i];
			di->refs++;
		}
	}

	if (v ->alias_indexes) {
		v2->alias_indexes = scf_vector_clone(v->alias_indexes);

		if (!v2->alias_indexes) {
			scf_dn_status_free(v2);
			return NULL;
		}

		for (i = 0; i < v2->alias_indexes->size; i++) {
			di = v2->alias_indexes->data[i];
			di->refs++;
		}
	}

	v2->dag_node     = v->dag_node;
	v2->dereference  = v->dereference;

	v2->alias        = v->alias;
	v2->alias_type   = v->alias_type;

	v2->active       = v->active;
	v2->inited       = v->inited;
	v2->updated      = v->updated;
	return v2;
}

void scf_dn_status_free(scf_dn_status_t* v)
{
	scf_dn_index_t* di;
	int i;

	if (v) {
		if (v->dn_indexes) {
			for (i = 0; i < v->dn_indexes->size; i++)
				scf_dn_index_free(v->dn_indexes->data[i]);

			scf_vector_free(v->dn_indexes);
		}

		if (v->alias_indexes) {
			for (i = 0; i < v->alias_indexes->size; i++)
				scf_dn_index_free(v->alias_indexes->data[i]);

			scf_vector_free(v->alias_indexes);
		}

		free(v);
		v = NULL;
	}
}

void scf_dn_status_print(scf_dn_status_t* ds)
{
	scf_dn_index_t* di;
	scf_variable_t* v;
	int i;

	if (ds->dag_node) {
		v = ds->dag_node->var;
		printf("dn: v_%d_%d/%s ", v->w->line, v->w->pos, v->w->text->data);

		if (ds->dn_indexes) {
			for (i = ds->dn_indexes->size - 1; i >= 0; i--) {
				di = ds->dn_indexes->data[i];

				if (di->member)
					printf("->%s ", di->member->w->text->data);
				else
					printf("[%ld] ", di->index);
			}
		}
	}

	if (ds->alias) {
		v = ds->alias->var;
		printf(" alias: v_%d_%d/%s ", v->w->line, v->w->pos, v->w->text->data);

		if (ds->alias_indexes) {
			for (i = ds->alias_indexes->size - 1; i >= 0; i--) {
				di = ds->alias_indexes->data[i];

				if (di->member)
					printf("->%s ", di->member->w->text->data);
				else
					printf("[%ld] ", di->index);
			}
		}
	}

	printf(" alias_type: %d\n", ds->alias_type);
}

scf_dag_node_t* scf_dag_node_alloc(int type, scf_variable_t* var)
{
	scf_dag_node_t* dag_node = calloc(1, sizeof(scf_dag_node_t));
	if (!dag_node)
		return NULL;

	dag_node->type = type;
	if (var)
		dag_node->var = scf_variable_clone(var);
	else
		dag_node->var = NULL;

#if 0
	if (SCF_OP_LOGIC_AND == type) {
		scf_logw("dag_node: %#lx, dag_node->type: %d", 0xffff & (uintptr_t)dag_node, dag_node->type);
		if (var) {
			printf(", var: %p, var->type: %d", var, var->type);
			if (var->w)
				printf(", v_%d_%d/%s", var->w->line, var->w->pos, var->w->text->data);
			else {
				//printf(", v_%#lx", 0xffff & (uintptr_t)var);
			}
		}
		printf("\n");
	}
#endif
	return dag_node;
}

int scf_dag_node_add_child(scf_dag_node_t* parent, scf_dag_node_t* child)
{
	if (!parent || !child)
		return -EINVAL;

	if (!parent->childs) {
		parent->childs = scf_vector_alloc();
		if (!parent->childs)
			return -ENOMEM;
	}

	if (!child->parents) {
		child->parents = scf_vector_alloc();
		if (!child->parents)
			return -ENOMEM;
	}

	int ret = scf_vector_add(parent->childs, child);
	if (ret < 0)
		return ret;

	ret = scf_vector_add(child->parents, parent);
	if (ret < 0) {
		scf_vector_del(parent->childs, child);
		return ret;
	}

	return 0;
}

void scf_dag_node_free(scf_dag_node_t* dag_node)
{
	if (dag_node) {
		if (dag_node->var)
			scf_variable_free(dag_node->var);

		if (dag_node->parents)
			scf_vector_free(dag_node->parents);

		if (dag_node->childs)
			scf_vector_free(dag_node->childs);

		free(dag_node);
		dag_node = NULL;
	}
}

static int _dn_status_cmp_dn_indexes(const void* p0, const void* p1,
		int (*cmp)(const scf_dn_index_t*, const scf_dn_index_t*))
{
	const scf_dn_status_t* ds0 = p0;
	const scf_dn_status_t* ds1 = p1;

	if (ds0->dag_node != ds1->dag_node)
		return -1;

	if (ds0->dn_indexes) {
		if (!ds1->dn_indexes)
			return -1;

		if (ds0->dn_indexes->size != ds1->dn_indexes->size)
			return -1;

		int i;
		for (i = 0; i < ds0->dn_indexes->size; i++) {

			scf_dn_index_t* di0 = ds0->dn_indexes->data[i];
			scf_dn_index_t* di1 = ds1->dn_indexes->data[i];

			if (!cmp(di0, di1))
				return -1;
		}
		return 0;
	} else if (ds1->dn_indexes)
		return -1;
	return 0;
}

int scf_dn_status_cmp_same_dn_indexes(const void* p0, const void* p1)
{
	return _dn_status_cmp_dn_indexes(p0, p1, scf_dn_index_same);
}

int scf_dn_status_cmp_like_dn_indexes(const void* p0, const void* p1)
{
	return _dn_status_cmp_dn_indexes(p0, p1, scf_dn_index_like);
}

int scf_dn_status_cmp_alias(const void* p0, const void* p1)
{
	const scf_dn_status_t* v0 = p0;
	const scf_dn_status_t* v1 = p1;

	assert(SCF_DN_ALIAS_NULL != v0->alias_type);
	assert(SCF_DN_ALIAS_NULL != v1->alias_type);

	if (v0->alias_type != v1->alias_type)
		return -1;

	if (SCF_DN_ALIAS_ALLOC == v0->alias_type)
		return 0;

	if (v0->alias != v1->alias)
		return -1;

	switch (v0->alias_type) {

		case SCF_DN_ALIAS_VAR:
			return 0;
			break;

		case SCF_DN_ALIAS_ARRAY:
		case SCF_DN_ALIAS_MEMBER:
			if (v0->alias_indexes) {
				if (!v1->alias_indexes)
					return -1;

				if (v0->alias_indexes->size != v1->alias_indexes->size)
					return -1;

				int i;
				for (i = 0; i < v0->alias_indexes->size; i++) {

					scf_dn_index_t* di0 = v0->alias_indexes->data[i];
					scf_dn_index_t* di1 = v1->alias_indexes->data[i];

					if (!scf_dn_index_like(di0, di1))
						return -1;
				}
				return 0;
			} else if (v1->alias_indexes)
				return -1;
			return 0;
			break;

		default:
			break;
	};

	return -1;
}

void scf_dag_node_free_list(scf_list_t* dag_list_head)
{
	scf_list_t* l;

	for (l = scf_list_head(dag_list_head); l != scf_list_sentinel(dag_list_head); ) {

		scf_dag_node_t* dn = scf_list_data(l, scf_dag_node_t, list);

		l = scf_list_next(l);

		scf_list_del(&dn->list);

		scf_dag_node_free(dn);
		dn = NULL;
	}
}

int scf_dag_node_same(scf_dag_node_t* dag_node, const scf_node_t* node)
{
	int i;

	if (dag_node->type != node->type)
		return 0;

	if (SCF_OP_ADDRESS_OF == node->type) {
		scf_logd("type: %d, %d, node: %p, %p\n", dag_node->type, node->type, dag_node, node);
		scf_logd("var: %p, %p\n", dag_node->var, node->var);
//		scf_logi("var: %#lx, %#lx\n", 0xffff & (uintptr_t)dag_node->var, 0xffff & (uintptr_t)node->var);
	}

	if (scf_type_is_var(node->type)) {
		if (dag_node->var == node->var)
			return 1;
		else
			return 0;
	}

	if (SCF_OP_LOGIC_AND == node->type
			|| SCF_OP_LOGIC_OR == node->type
			|| SCF_OP_INC      == node->type
			|| SCF_OP_DEC      == node->type
			|| SCF_OP_INC_POST == node->type
			|| SCF_OP_DEC_POST == node->type) {
		if (dag_node->var == _scf_operand_get((scf_node_t*)node))
			return 1;
		return 0;
	}

	if (!dag_node->childs)
		return 0;

	if (SCF_OP_TYPE_CAST == node->type) {
		scf_dag_node_t* dn0 = dag_node->childs->data[0];
		scf_variable_t* vn1 = _scf_operand_get(node->nodes[1]);

		if (dn0->var == vn1
				&& scf_variable_same_type(dag_node->var, node->result))
			return 1;
		else {
			scf_loge("var: %#lx, %#lx, type: %d, %d, node: %#lx, %#lx, same: %d\n",
					0xffff & (uintptr_t)dn0->var,
					0xffff & (uintptr_t)vn1,
					dag_node->var->type, node->result->type,
					0xffff & (uintptr_t)dag_node,
					0xffff & (uintptr_t)node,
					scf_variable_same_type(dag_node->var, node->result)
					);
			return 0;
		}
	} else if (SCF_OP_ARRAY_INDEX == node->type) {
		assert(3 == dag_node->childs->size);
		assert(2 == node->nb_nodes);
		goto cmp_childs;

	} else if (SCF_OP_ADDRESS_OF == node->type) {
		assert(1 == node->nb_nodes);

		if (SCF_OP_ARRAY_INDEX == node->nodes[0]->type) {
			assert(2 == node->nodes[0]->nb_nodes);

			if (!dag_node->childs || 3 != dag_node->childs->size)
				return 0;

			node = node->nodes[0];
			goto cmp_childs;

		} else if (SCF_OP_POINTER == node->nodes[0]->type) {
			assert(2 == node->nodes[0]->nb_nodes);

			if (!dag_node->childs || 2 != dag_node->childs->size)
				return 0;

			node = node->nodes[0];
			goto cmp_childs;
		}
	}

	if (dag_node->childs->size != node->nb_nodes) {
		if (SCF_OP_ADDRESS_OF == node->type) {
			scf_loge("node: %p, %p, size: %d, %d\n", dag_node, node, dag_node->childs->size, node->nb_nodes);
		}
		return 0;
	}

cmp_childs:
	for (i = 0; i < node->nb_nodes; i++) {
		scf_dag_node_t*	dag_child = dag_node->childs->data[i];
		scf_node_t*		child = node->nodes[i];

		while (SCF_OP_EXPR == child->type)
			child = child->nodes[0];

		if (0 == scf_dag_node_same(dag_child, child))
			return 0;
	}

	for (i = 0; i < dag_node->childs->size; i++) {
		scf_dag_node_t*	child  = dag_node->childs->data[i];
		scf_dag_node_t* parent = NULL;

		assert(child->parents);

		int j;
		for (j = 0; j < child->parents->size; j++) {
			if (dag_node == child->parents->data[j])
				break;
		}
		assert(j < child->parents->size);

		for (++j; j < child->parents->size; j++) {
			parent  = child->parents->data[j];

			if (scf_type_is_assign(parent->type)
					|| SCF_OP_INC       == parent->type
					|| SCF_OP_DEC       == parent->type
					|| SCF_OP_3AC_SETZ  == parent->type
					|| SCF_OP_3AC_SETNZ == parent->type
					|| SCF_OP_3AC_SETLT == parent->type
					|| SCF_OP_3AC_SETLE == parent->type
					|| SCF_OP_3AC_SETGT == parent->type
					|| SCF_OP_3AC_SETGE == parent->type) {
				return 0;
			}
		}
	}

	return 1;
}

scf_dag_node_t* scf_dag_find_node(scf_list_t* h, const scf_node_t* node)
{
	scf_node_t* origin = (scf_node_t*)node;

	while (SCF_OP_EXPR == origin->type)
		origin = origin->nodes[0];

	scf_list_t* l;
	for (l = scf_list_tail(h); l != scf_list_sentinel(h); l = scf_list_prev(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (scf_dag_node_same(dag_node, origin))
			return dag_node;
	}

	return NULL;
}

int scf_dag_get_node(scf_list_t* h, const scf_node_t* node, scf_dag_node_t** pp)
{
	scf_dag_node_t* dag_node = scf_dag_find_node(h, node);

	if (!dag_node) {
		dag_node = scf_dag_node_alloc(node->type, _scf_operand_get((scf_node_t*)node));
		if (!dag_node)
			return -ENOMEM;

		scf_list_add_tail(h, &dag_node->list);
	}

	*pp = dag_node;
	return 0;
}

int scf_dag_dn_same(scf_dag_node_t* dn0, scf_dag_node_t* dn1)
{
	int i;

	if (dn0->type != dn1->type)
		return 0;

	if (scf_type_is_var(dn0->type)) {
		if (dn0->var == dn1->var)
			return 1;
		else
			return 0;
	}

	if (!dn0->childs) {
		if (dn1->childs)
			return 0;

		scf_logd("dn0: %p, %p, dn1: %p, %p, var: %p, %p\n", dn0, dn0->childs, dn1, dn1->childs, dn0->var, dn1->var);
		if (dn0->var == dn1->var)
			return 1;
		return 0;
	} else if (!dn1->childs)
		return 0;

	if (dn0->childs->size != dn1->childs->size) {
		scf_logd("dn0: %p, %d, dn1: %p, %d\n", dn0, dn0->childs->size, dn1, dn1->childs->size);
		return 0;
	}

	for (i = 0; i < dn0->childs->size; i++) {
		scf_dag_node_t*	child0 = dn0->childs->data[i];
		scf_dag_node_t*	child1 = dn1->childs->data[i];

		if (0 == scf_dag_dn_same(child0, child1))
			return 0;
	}
	return 1;
}

scf_dag_node_t* scf_dag_find_dn(scf_list_t* h, const scf_dag_node_t* dn0)
{
	scf_list_t* l;
	for (l = scf_list_tail(h); l != scf_list_sentinel(h); l = scf_list_prev(l)) {

		scf_dag_node_t* dn1 = scf_list_data(l, scf_dag_node_t, list);

		if (SCF_OP_ADDRESS_OF == dn0->type && SCF_OP_ADDRESS_OF == dn1->type) {
			scf_logd("origin dn0: %p, %s, dn1: %p, %s\n",
					dn0, dn0->var->w->text->data,
					dn1, dn1->var->w->text->data);
		}

		if (scf_dag_dn_same(dn1, (scf_dag_node_t*)dn0))
			return dn1;
	}
	return NULL;
}

int scf_dag_get_dn(scf_list_t* h, const scf_dag_node_t* dn0, scf_dag_node_t** pp)
{
	scf_dag_node_t* dn1 = scf_dag_find_dn(h, dn0);

	if (!dn1) {
		dn1 = scf_dag_node_alloc(dn0->type, dn0->var);
		if (!dn1)
			return -ENOMEM;

		scf_list_add_tail(h, &dn1->list);
	}

	*pp = dn1;
	return 0;
}

int scf_dag_find_roots(scf_list_t* h, scf_vector_t* roots)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->parents) {
			int ret = scf_vector_add(roots, dag_node);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_dag_find_leafs(scf_list_t* h, scf_vector_t* leafs)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->childs) {
			int ret = scf_vector_add_unique(leafs, dag_node);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_dag_root_find_leafs(scf_dag_node_t* root, scf_vector_t* leafs)
{
	if (!root->childs)
		return scf_vector_add_unique(leafs, root);

	int i;
	for (i = 0; i < root->childs->size; i++) {

		scf_dag_node_t* child = root->childs->data[i];

		int ret = scf_dag_root_find_leafs(child, leafs);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int scf_dag_vector_find_leafs(scf_vector_t* roots, scf_vector_t* leafs)
{
	int i;
	for (i = 0; i < roots->size; i++) {

		scf_dag_node_t* root = roots->data[i];

		int ret = scf_dag_root_find_leafs(root, leafs);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int _dn_status_index(scf_vector_t* indexes, scf_dag_node_t* dn_index, int type)
{
	scf_dn_index_t* di;

	di = scf_dn_index_alloc();
	if (!di)
		return -ENOMEM;

	if (SCF_OP_ARRAY_INDEX == type) {

		if (scf_variable_const(dn_index->var))
			di->index = dn_index->var->data.i;
		else
			di->index = -1;

	} else if (SCF_OP_POINTER == type) {
		di->member = dn_index->var;
	} else {
		scf_dn_index_free(di);
		return -1;
	}

	int ret = scf_vector_add(indexes, di);
	if (ret < 0) {
		scf_dn_index_free(di);
		return ret;
	}
	return 0;
}

int scf_dn_status_index(scf_dn_status_t* ds, scf_dag_node_t* dn_index, int type)
{
	return _dn_status_index(ds->dn_indexes, dn_index, type);
}

int scf_dn_status_alias_index(scf_dn_status_t* ds, scf_dag_node_t* dn_index, int type)
{
	return _dn_status_index(ds->alias_indexes, dn_index, type);
}

void scf_dn_status_vector_clear_by_ds(scf_vector_t* vec, scf_dn_status_t* ds)
{
	scf_dn_status_t* ds2;

	while (1) {
		ds2 = scf_vector_find_cmp(vec, ds, scf_dn_status_cmp_same_dn_indexes);
		if (!ds2)
			break;
		assert(0 == scf_vector_del(vec, ds2));
	}
}

void scf_dn_status_vector_clear_by_dn(scf_vector_t* vec, scf_dag_node_t* dn)
{
	scf_dn_status_t* ds;

	while (1) {
		ds = scf_vector_find_cmp(vec, dn, scf_dn_status_cmp);
		if (!ds)
			break;
		assert(0 == scf_vector_del(vec, ds));
	}
}
