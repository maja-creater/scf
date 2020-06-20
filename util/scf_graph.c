#include"scf_graph.h"

scf_graph_t* scf_graph_alloc()
{
	scf_graph_t* graph = calloc(1, sizeof(scf_graph_t));
	assert(graph);

	graph->nodes = scf_vector_alloc();
	return graph;
}

void scf_graph_free(scf_graph_t* graph)
{
	if (graph) {
		int i;
		for (i = 0; i < graph->nodes->size; i++) {
			scf_graph_node_t* node = graph->nodes->data[i];

			scf_graph_node_free(node);
		}
		scf_vector_free(graph->nodes);
		free(graph);
		graph = NULL;
	}
}

scf_graph_node_t* scf_graph_node_alloc()
{
	scf_graph_node_t* node = calloc(1, sizeof(scf_graph_node_t));
	assert(node);

	node->neighbors = scf_vector_alloc();
	return node;
}

void scf_graph_node_free(scf_graph_node_t* node)
{
	if (node) {
		scf_vector_free(node->neighbors);
		free(node);
		node = NULL;
	}
}

void scf_graph_node_print(scf_graph_node_t* node)
{
	if (node) {
		printf("node: %p, color: %d\n", node, node->color);
		int j;
		for (j = 0; j < node->neighbors->size; j++) {
			scf_graph_node_t* neighbor = node->neighbors->data[j];
			printf("neighbor: %p, color: %d\n", neighbor, neighbor->color);
		}
		printf("\n");
	}
}

void scf_graph_make_edge(scf_graph_node_t* node1, scf_graph_node_t* node2)
{
	scf_vector_add(node1->neighbors, node2);
}

void scf_graph_delete_node(scf_graph_t* graph, scf_graph_node_t* node)
{
	int j;
	for (j = 0; j < node->neighbors->size; j++) {
		scf_graph_node_t* neighbor = node->neighbors->data[j];
		assert(0 == scf_vector_del(neighbor->neighbors, node));
	}

	scf_vector_del(graph->nodes, node);
}

void scf_graph_add_node(scf_graph_t* graph, scf_graph_node_t* node)
{
	int j;
	for (j = 0; j < node->neighbors->size; j++) {
		scf_graph_node_t* neighbor = node->neighbors->data[j];
		assert(scf_vector_find(graph->nodes, neighbor));
		scf_vector_add(neighbor->neighbors, node);
	}

	scf_vector_add(graph->nodes, node);
}

void _scf_graph_k_color_delete(scf_graph_t* graph, int k, scf_vector_t* deleted_nodes)
{
	while (graph->nodes->size > 0) {

		int nb_deleted = 0;
		int i = 0;
		while (i < graph->nodes->size) {
			scf_graph_node_t* node = graph->nodes->data[i];

			if (node->neighbors->size >= k) {
				i++;
				continue;
			}

			//printf("%s(),%d, node: %p, neighbors: %d, k: %d\n", __func__, __LINE__, node, node->neighbors->size, k);
			scf_graph_delete_node(graph, node);
			scf_vector_add(deleted_nodes, node);
			nb_deleted++;
		}

		if (0 == nb_deleted) {
			break;
		}
	}
}

void _scf_graph_k_color_fill(scf_graph_t* graph, int k, scf_vector_t* colors, scf_vector_t* deleted_nodes)
{
	assert(k == colors->size);

	int i;
	for (i = deleted_nodes->size - 1; i >= 0; i--) {
		scf_graph_node_t* node = deleted_nodes->data[i];
//		printf("%s(),%d, node: %p, neighbor: %d, k: %d\n", __func__, __LINE__, node, node->neighbors->size, k);
		assert(node->neighbors->size < k);

		scf_vector_t* cloned_colors = scf_vector_clone(colors);

		int j;
		for (j = 0; j < node->neighbors->size; j++) {
			scf_graph_node_t* neighbor = node->neighbors->data[j];

			if (neighbor->color > 0) {
				scf_vector_del(cloned_colors, (void*)(intptr_t)neighbor->color);
			}
			scf_vector_add(neighbor->neighbors, node);
		}
		assert(cloned_colors->size > 0);

		node->color = (int)(intptr_t)(cloned_colors->data[0]);
		scf_vector_add(graph->nodes, node);

		scf_vector_free(cloned_colors);
		cloned_colors = NULL;
	}
}

int _scf_graph_k_color_find_not_neighbor(scf_graph_t* graph, int k, scf_vector_t* pair)
{
	assert(graph->nodes->size >= k);

	int i;
	for (i = 0; i < graph->nodes->size; i++) {
		scf_graph_node_t* node0 = graph->nodes->data[i];

		if (node0->neighbors->size > k)
			continue;

		scf_graph_node_t* node1 = NULL; 
		int j;
		for (j = i + 1; j < graph->nodes->size; j++) {
			node1 = graph->nodes->data[j];

			if (!scf_vector_find(node0->neighbors, node1)) {
				assert(!scf_vector_find(node1->neighbors, node0));
				break;
			}

			node1 = NULL;
		}

		if (node1) {
			scf_vector_add(pair, node0);
			scf_vector_add(pair, node1);
			return 0;
		}
	}
	return -1;
}

int scf_graph_k_color(scf_graph_t* graph, int k, scf_vector_t* colors)
{
	assert(k == colors->size);

	scf_vector_t* deleted_nodes = scf_vector_alloc();

	printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
	_scf_graph_k_color_delete(graph, k, deleted_nodes);

	if (0 == graph->nodes->size) {

		_scf_graph_k_color_fill(graph, k, colors, deleted_nodes);

		scf_vector_free(deleted_nodes);
		deleted_nodes = NULL;
		printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
		return 0;
	}

	assert(graph->nodes->size > 0);
	assert(graph->nodes->size >= k);

	scf_vector_t* not_neighbor = scf_vector_alloc();

	if (0 == _scf_graph_k_color_find_not_neighbor(graph, k, not_neighbor)) {

		assert(2 == not_neighbor->size);

		scf_graph_node_t* node0 = not_neighbor->data[0];
		scf_graph_node_t* node1 = not_neighbor->data[1];

		node0->color = (int)(intptr_t)(colors->data[0]);
		node1->color = node0->color;

		printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
		scf_graph_delete_node(graph, node0);
		scf_graph_delete_node(graph, node1);

		scf_vector_t* cloned_colors = scf_vector_clone(colors);
		scf_vector_del(cloned_colors, (void*)(intptr_t)node0->color);

		scf_graph_k_color(graph, k - 1, cloned_colors);

		scf_graph_add_node(graph, node0);
		scf_graph_add_node(graph, node1);
		printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
	} else {
		int i;
		scf_graph_node_t* node_max = NULL;
		int max_neighbors = 0;

		for (i = 0; i < graph->nodes->size; i++) {
			scf_graph_node_t* node = graph->nodes->data[i];

			if (!node_max) {
				node_max = node;
				max_neighbors = node->neighbors->size;

			} else if (max_neighbors < node->neighbors->size) {
				node_max = node;
				max_neighbors = node->neighbors->size;
			}
			printf("neighbors: %d\n", node->neighbors->size);
		}
		printf("max_neighbors: %d\n", max_neighbors);

		printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
		scf_graph_delete_node(graph, node_max);
		node_max->color = -1;

		scf_graph_k_color(graph, k, colors);

		scf_graph_add_node(graph, node_max);
		printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
	}

	_scf_graph_k_color_fill(graph, k, colors, deleted_nodes);

	scf_vector_free(deleted_nodes);
	deleted_nodes = NULL;
	printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);
	return 0;
}


















