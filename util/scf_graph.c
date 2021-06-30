#include"scf_graph.h"

scf_graph_t* scf_graph_alloc()
{
	scf_graph_t* graph = calloc(1, sizeof(scf_graph_t));
	if (!graph)
		return NULL;

	graph->nodes = scf_vector_alloc();
	if (!graph->nodes) {
		free(graph);
		graph = NULL;
		return NULL;
	}
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
	if (!node)
		return NULL;

	node->neighbors = scf_vector_alloc();
	if (!node->neighbors) {
		free(node);
		node = NULL;
		return NULL;
	}
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
		printf("node: %p, color: %d\n", node, (int)node->color);
		int j;
		for (j = 0; j < node->neighbors->size; j++) {
			scf_graph_node_t* neighbor = node->neighbors->data[j];
			printf("neighbor: %p, color: %d\n", neighbor, (int)neighbor->color);
		}
		printf("\n");
	}
}

int scf_graph_make_edge(scf_graph_node_t* node1, scf_graph_node_t* node2)
{
	if (!node1 || !node2)
		return -EINVAL;

	return scf_vector_add(node1->neighbors, node2);
}

int scf_graph_delete_node(scf_graph_t* graph, scf_graph_node_t* node)
{
	if (!graph || !node)
		return -EINVAL;

	scf_graph_node_t* neighbor;
	int j;

	for (j = 0; j < node->neighbors->size; j++) {
		neighbor  = node->neighbors->data[j];

		scf_logd("node %p, neighbor: %p, %d\n", node, neighbor, neighbor->neighbors->size);

		// it may be not 2 side graph
		scf_vector_del(neighbor->neighbors, node);
	}

	if (0 != scf_vector_del(graph->nodes, node))
		return -1;
	return 0;
}

int scf_graph_add_node(scf_graph_t* graph, scf_graph_node_t* node)
{
	int j;
	for (j = 0; j < node->neighbors->size; j++) {

		scf_graph_node_t* neighbor = node->neighbors->data[j];

		if (scf_vector_find(graph->nodes, neighbor)) {
		
			if (!scf_vector_find(neighbor->neighbors, node)) {

				if (0 != scf_vector_add(neighbor->neighbors, node))
					return -1;
			} else
				scf_logw("node %p already in neighbor: %p\n", node, neighbor);
		} else
			scf_logw("neighbor: %p of node: %p not in graph\n", neighbor, node);
	}

	if (0 != scf_vector_add(graph->nodes, node))
		return -1;
	return 0;
}

static int _kcolor_delete(scf_graph_t* graph, int k, scf_vector_t* deleted_nodes)
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

			if (0 != scf_graph_delete_node(graph, node)) {
				scf_loge("scf_graph_delete_node\n");
				return -1;
			}

			if (0 != scf_vector_add(deleted_nodes, node)) {
				scf_loge("scf_graph_delete_node\n");
				return -1;
			}
			nb_deleted++;
		}

		if (0 == nb_deleted)
			break;
	}

	return 0;
}

static int _kcolor_fill(scf_graph_t* graph, scf_vector_t* colors, scf_vector_t* deleted_nodes)
{
	int k = colors->size;
	int i;
	int j;
	scf_vector_t* colors2 = NULL;

	for (i = deleted_nodes->size - 1; i >= 0; i--) {
		scf_graph_node_t* node = deleted_nodes->data[i];

		assert(node->neighbors->size < k);

		colors2 = scf_vector_clone(colors);
		if (!colors2)
			return -ENOMEM;

		scf_logd("node: %p, neighbor: %d, k: %d\n", node, node->neighbors->size, k);

		for (j = 0; j < node->neighbors->size; j++) {
			scf_graph_node_t* neighbor = node->neighbors->data[j];

			if (neighbor->color > 0)
				scf_vector_del(colors2, (void*)neighbor->color);

			if (0 != scf_vector_add(neighbor->neighbors, node))
				goto error;
		}

		assert(colors2->size >= 0);
		if (0 == colors2->size)
			goto error;

		node->color = (intptr_t)(colors2->data[0]);
		if (0 != scf_vector_add(graph->nodes, node))
			goto error;

		scf_vector_free(colors2);
		colors2 = NULL;
	}

	return 0;

error:
	scf_vector_free(colors2);
	colors2 = NULL;
	return -1;
}

static int _kcolor_find_not_neighbor(scf_graph_t* graph, int k, scf_graph_node_t** pp0, scf_graph_node_t** pp1)
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
			*pp0 = node0;
			*pp1 = node1;
			return 0;
		}
	}
	return -1;
}

static scf_graph_node_t* _max_neighbors(scf_graph_t* graph)
{
	scf_graph_node_t* node_max = NULL;
	int max = 0;
	int i;
	for (i = 0; i < graph->nodes->size; i++) {
		scf_graph_node_t* node = graph->nodes->data[i];

		if (!node_max || max < node->neighbors->size) {
			node_max = node;
			max      = node->neighbors->size;
		}
	}

	scf_logi("max_neighbors: %d\n", max);

	return node_max;
}

int scf_graph_kcolor(scf_graph_t* graph, scf_vector_t* colors)
{
	if (!graph || !colors || 0 == colors->size)
		return -EINVAL;

	int k   = colors->size;
	int ret = -1;

	scf_vector_t* colors2       = NULL;
	scf_vector_t* deleted_nodes = scf_vector_alloc();
	if (!deleted_nodes)
		return -ENOMEM;

	scf_logd("graph->nodes->size: %d, k: %d\n", graph->nodes->size, k);

	ret = _kcolor_delete(graph, k, deleted_nodes);
	if (ret < 0)
		goto error;

	if (0 == graph->nodes->size) {
		ret = _kcolor_fill(graph, colors, deleted_nodes);
		if (ret < 0)
			goto error;

		scf_vector_free(deleted_nodes);
		deleted_nodes = NULL;

		scf_logd("graph->nodes->size: %d\n", graph->nodes->size);
		return 0;
	}

	assert(graph->nodes->size > 0);
	assert(graph->nodes->size >= k);

	scf_graph_node_t* node0 = NULL;
	scf_graph_node_t* node1 = NULL;

	if (0 == _kcolor_find_not_neighbor(graph, k, &node0, &node1)) {
		assert(node0);
		assert(node1);

		node0->color = (intptr_t)(colors->data[0]);
		node1->color = node0->color;

		ret = scf_graph_delete_node(graph, node0);
		if (ret < 0)
			goto error;

		ret = scf_graph_delete_node(graph, node1);
		if (ret < 0)
			goto error;

		assert(!colors2);
		colors2 = scf_vector_clone(colors);
		if (!colors2) {
			ret = -ENOMEM;
			goto error;
		}

		ret = scf_vector_del(colors2, (void*)node0->color);
		if (ret < 0)
			goto error;

		ret = scf_graph_kcolor(graph, colors2);
		if (ret < 0)
			goto error;

		ret = scf_graph_add_node(graph, node0);
		if (ret < 0)
			goto error;

		ret = scf_graph_add_node(graph, node1);
		if (ret < 0)
			goto error;

		scf_vector_free(colors2);
		colors2 = NULL;
	} else {
		scf_graph_node_t* node_max = _max_neighbors(graph);
		assert(node_max);

		ret = scf_graph_delete_node(graph, node_max);
		if (ret < 0)
			goto error;
		node_max->color = -1;

		ret = scf_graph_kcolor(graph, colors);
		if (ret < 0)
			goto error;

		ret = scf_graph_add_node(graph, node_max);
		if (ret < 0)
			goto error;
	}

	ret = _kcolor_fill(graph, colors, deleted_nodes);
	if (ret < 0)
		goto error;

	scf_vector_free(deleted_nodes);
	deleted_nodes = NULL;

	scf_logd("graph->nodes->size: %d\n", graph->nodes->size);
	return 0;

error:
	if (colors2)
		scf_vector_free(colors2);

	scf_vector_free(deleted_nodes);
	return ret;
}

