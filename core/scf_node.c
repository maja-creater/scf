#include"scf_node.h"

scf_variable_t* _scf_operand_get(scf_node_t* node)
{
	if (scf_type_is_var(node->type))
		return node->var;
	else if (scf_type_is_operator(node->type))
		return node->result;

	return NULL;
}

scf_function_t* _scf_function_get(scf_node_t* node)
{
	while (node) {
		if (SCF_FUNCTION == node->type)
			return (scf_function_t*)node;

		node = node->parent;
	}
	return NULL;
}

scf_node_t* scf_node_alloc(scf_lex_word_t* w, int type, scf_variable_t* var)
{
	scf_node_t* node = calloc(1, sizeof(scf_node_t));
	if (!node) {
		scf_loge("node alloc failed\n");
		return NULL;
	}

	if (scf_type_is_var(type)) {
		node->var = scf_variable_clone(var);
		if (!node->var) {
			scf_loge("node var clone failed\n");
			goto _failed;
		}
	} else {
		node->w = scf_lex_word_clone(w);
		if (!node->w) {
			scf_loge("node word clone failed\n");
			goto _failed;
		}
	}

	node->type = type;

	scf_logd("%s(),%d, node: %p, node->type: %d\n", __func__, __LINE__, node, node->type);
	return node;

_failed:
	free(node);
	node = NULL;
	return NULL;
}

scf_node_t*	scf_node_alloc_label(scf_label_t* l)
{
	scf_node_t* node = calloc(1, sizeof(scf_node_t));
	if (!node) {
		scf_loge("node alloc failed\n");
		return NULL;
	}

	node->type  = SCF_LABEL;
	node->label = l;
	return node;
}

int scf_node_add_child(scf_node_t* parent, scf_node_t* child)
{
	if (!parent) {
		scf_loge("invalid, parent is NULL\n");
		return -1;
	}

	void* p = realloc(parent->nodes, sizeof(scf_node_t*) * (parent->nb_nodes + 1));
	if (!p) {
		scf_loge("realloc failed\n");
		return -1;
	}

	parent->nodes = p;
	parent->nodes[parent->nb_nodes++] = child;

	if (child)
		child->parent = parent;

	return 0;
}

void scf_node_free_data(scf_node_t* node)
{
	if (!node)
		return;

	if (scf_type_is_var(node->type)) {
		if (node->var) {
			scf_variable_free(node->var);
			node->var = NULL;
		}
	} else if (SCF_LABEL == node->type) {
		if (node->label) {
			scf_label_free(node->label);
			node->label = NULL;
		}
	} else {
		if (node->w) {
			scf_lex_word_free(node->w);
			node->w = NULL;
		}
	}

	if (node->result) {
		scf_variable_free(node->result);
		node->result = NULL;
	}

	int i;
	for (i = 0; i < node->nb_nodes; i++) {
		if (node->nodes[i]) {
			scf_node_free(node->nodes[i]);
			node->nodes[i] = NULL;
		}
	}
	node->nb_nodes = 0;

	if (node->nodes) {
		free(node->nodes);
		node->nodes = NULL;
	}
}

void scf_node_free(scf_node_t* node)
{
	if (!node)
		return;

	scf_node_free_data(node);

	node->parent = NULL;

	free(node);
	node = NULL;
}

// BFS search
void* scf_node_search_bfs(scf_node_t* root, scf_node_find_pt find, void* arg, scf_vector_t* vec)
{
	assert(root);

	// BFS search
	scf_vector_t* queue   = scf_vector_alloc();
	scf_vector_t* checked = scf_vector_alloc();

	scf_vector_add(queue, root);

	void* result = NULL;
	int i = 0;
	while (i < queue->size) {
		scf_node_t* node = queue->data[i];

		int j;
		for (j = 0; j < checked->size; j++) {
			if (node == checked->data[j]) {
				break;
			}
		}

		if (j < checked->size) {
			i++;
			continue;
		}

		result = find(node, arg, vec);
		if (result) {
			if (!vec)
				break;
		}
		scf_vector_add(checked, node);

		for (j = 0; j < node->nb_nodes; j++) {
			assert(node->nodes);
			scf_vector_add(queue, node->nodes[j]);
		}

		i++;
	}

	scf_vector_free(queue);
	scf_vector_free(checked);
	queue   = NULL;
	checked = NULL;
	return result;
}

