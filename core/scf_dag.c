#include"scf_dag.h"

scf_dag_node_t* scf_dag_node_alloc(int type, scf_variable_t* var)
{
	scf_dag_node_t* dag_node = calloc(1, sizeof(scf_dag_node_t));
	assert(dag_node);

	dag_node->type = type;
	dag_node->var = var;
	if (var)
		var->refs++;

	printf("dag_node: %p, dag_node->type: %d", dag_node, dag_node->type);
	if (var) {
		printf(", w: %s, line: %d, pos: %d", var->w->text->data, var->w->line, var->w->pos);
	}
	printf("\n");
	return dag_node;
}

int scf_dag_node_add_child(scf_dag_node_t* parent, scf_dag_node_t* child)
{
	assert(parent);
	assert(child);

	if (!parent->nodes)
		parent->nodes = scf_vector_alloc();

	if (!child->parents)
		child->parents = scf_vector_alloc();

	scf_vector_add(parent->nodes, child);
	scf_vector_add(child->parents, parent);
	return 0;
}

void scf_dag_node_free(scf_dag_node_t* dag_node)
{
	assert(dag_node);
	scf_variable_free(dag_node->var);
	dag_node->var = NULL;

	if (dag_node->parents)
		scf_vector_free(dag_node->parents);

	if (dag_node->nodes)
		scf_vector_free(dag_node->nodes);

	free(dag_node);
	dag_node = NULL;
}

int scf_dag_node_same(const scf_dag_node_t* dag_node, const scf_node_t* node)
{
	if (dag_node->type != node->type) {
	//	printf("%s(),%d, type: %d, %d\n", __func__, __LINE__, dag_node->type, node->type);
		return 0;
	}

	if (scf_type_is_var(node->type)) {
		if (dag_node->var == node->var) {
			return 1;
		} else {
	//		printf("%s(),%d, var: %p, %p\n", __func__, __LINE__, dag_node->var, node->var);
			return 0;
		}
	}

	if (!dag_node->nodes)
		return 0;

	if (dag_node->nodes->size != node->nb_nodes)
		return 0;

	int i;
	for (i = 0; i < node->nb_nodes; i++) {
		scf_dag_node_t*	dag_child = dag_node->nodes->data[i];
		scf_node_t*		node_child = node->nodes[i];

		while (SCF_OP_EXPR == node_child->type)
			node_child = node_child->nodes[0];

		if (0 == scf_dag_node_same(dag_child, node_child))
			return 0;
	}

	return 1;
}

scf_dag_node_t* scf_list_find_dag_node(scf_list_t* h, const scf_node_t* node)
{
	scf_node_t* origin = (scf_node_t*)node;

	while (SCF_OP_EXPR == origin->type)
		origin = origin->nodes[0];

	scf_list_t* l;
	for (l = scf_list_tail(h); l != scf_list_sentinel(h); l = scf_list_prev(l)) {

		const scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (scf_dag_node_same(dag_node, origin))
			return (scf_dag_node_t*)dag_node;
	}

	return NULL;
}

int scf_dag_find_roots(scf_list_t* h, scf_vector_t* roots)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->parents)
			scf_vector_add(roots, dag_node);
	}
	return 0;
}

int scf_dag_find_leafs(scf_list_t* h, scf_vector_t* leafs)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->nodes && !scf_vector_find(leafs, dag_node)) {
			scf_vector_add(leafs, dag_node);
		}
	}
	return 0;
}

int scf_dag_root_find_leafs(scf_dag_node_t* root, scf_vector_t* leafs)
{
	if (!root->nodes) {
		if (!scf_vector_find(leafs, root)) {
			scf_vector_add(leafs, root);
		}
		return 0;
	}

	int i;
	for (i = 0; i < root->nodes->size; i++) {
		scf_dag_root_find_leafs((scf_dag_node_t*)(root->nodes->data[i]), leafs);
	}
	return 0;
}

int scf_dag_vector_find_leafs(scf_vector_t* roots, scf_vector_t* leafs)
{
	int i;
	for (i = 0; i < roots->size; i++) {
		scf_dag_root_find_leafs((scf_dag_node_t*)(roots->data[i]), leafs);
	}
	return 0;
}















