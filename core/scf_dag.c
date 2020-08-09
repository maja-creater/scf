#include"scf_dag.h"

scf_active_var_t* scf_active_var_alloc(scf_dag_node_t* dag_node)
{
	if (!dag_node)
		return NULL;

	scf_active_var_t* v = calloc(1, sizeof(scf_active_var_t));
	if (!v)
		return NULL;

	v->dag_node = dag_node;
	v->active   = dag_node->active;
	return v;
}

void scf_active_var_free(scf_active_var_t* v)
{
	if (v) {
		free(v);
		v = NULL;
	}
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

#if 1
	if (SCF_OP_TYPE_CAST == type) {
		scf_logi("dag_node: %#lx, dag_node->type: %d", 0xffff & (uintptr_t)dag_node, dag_node->type);
		if (var) {
			printf(", %d", var->type);
			if (var->w)
				printf(", w: %s, line: %d, pos: %d", var->w->text->data, var->w->line, var->w->pos);
			else
				printf(", v_%#lx", 0xffff & (uintptr_t)var);
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

int scf_dag_node_same(scf_dag_node_t* dag_node, const scf_node_t* node)
{
	int i;

	if (dag_node->type != node->type)
		return 0;

	if (SCF_OP_TYPE_CAST == node->type) {
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
	}

	if (dag_node->childs->size != node->nb_nodes)
		return 0;

cmp_childs:
	for (i = 0; i < node->nb_nodes; i++) {
		scf_dag_node_t*	dag_child = dag_node->childs->data[i];
		scf_node_t*		child = node->nodes[i];

		while (SCF_OP_EXPR == child->type)
			child = child->nodes[0];

		if (0 == scf_dag_node_same(dag_child, child))
			return 0;
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

