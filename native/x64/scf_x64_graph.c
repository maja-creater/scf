#include"scf_graph.h"
#include"scf_x64.h"

static int _x64_color_select(scf_graph_node_t* node, scf_vector_t* colors)
{
	x64_rcg_node_t* rn = node->data;

	if (!rn->dag_node) {
		assert(rn->reg);
		return rn->reg->color;
	}

	int i;
	for (i = 0; i < colors->size; i++) {
		intptr_t c     = (intptr_t)(colors->data[i]);
		int      bytes = X64_COLOR_BYTES(c);
		uint32_t type  = X64_COLOR_TYPE(c);

		if (bytes == rn->dag_node->var->size
				&& type == scf_type_is_float(rn->dag_node->var->type))
			return c;
	}

	return 0;
}

static int _x64_color_del(scf_vector_t* colors, intptr_t color)
{
	int i = 0;

	while (i < colors->size) {
		intptr_t c = (intptr_t)(colors->data[i]);

		if (X64_COLOR_CONFLICT(c, color)) {
			int ret = scf_vector_del(colors, (void*)c);
			if (ret < 0)
				return ret;
		} else {
			//scf_logi("c: %ld, color: %ld\n", c, color);
			i++;
		}
	}

	return 0;
}

static int _x64_kcolor_check(scf_graph_t* graph)
{
	int i;
	for (i = 0; i < graph->nodes->size; i++) {

		scf_graph_node_t* node = graph->nodes->data[i];
		x64_rcg_node_t*   rn   = node->data;

		if (rn->reg)
			assert(node->color > 0);

		if (0 == node->color)
			return -1;
	}

	return 0;
}

static int _x64_kcolor_delete(scf_graph_t* graph, int k, scf_vector_t* deleted_nodes)
{
	while (graph->nodes->size > 0) {

		int nb_deleted = 0;
		int i = 0;
		while (i < graph->nodes->size) {

			scf_graph_node_t* node = graph->nodes->data[i];
			x64_rcg_node_t*   rn   = node->data;

			scf_logd("graph->nodes->size: %d, neighbors: %d, k: %d, rn->reg: %p\n",
					graph->nodes->size, node->neighbors->size, k, rn->reg);

			if (!rn->dag_node) {
				assert(rn->reg);
				assert(node->color > 0);
			} else {
				if (node->neighbors->size >= k) {
					i++;
					continue;
				}
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

static int _x64_kcolor_fill(scf_graph_t* graph, int k, scf_vector_t* colors,
		scf_vector_t* deleted_nodes)
{
	//scf_logi("graph->nodes->size: %d\n", graph->nodes->size);
	int i;
	int j;
	scf_vector_t* colors2 = NULL;

	for (i = deleted_nodes->size - 1; i >= 0; i--) {
		scf_graph_node_t* node = deleted_nodes->data[i];
		x64_rcg_node_t*   rn   = node->data; 

		if (node->neighbors->size >= k)
			assert(rn->reg);

		colors2 = scf_vector_clone(colors);
		if (!colors2)
			return -ENOMEM;

		scf_logd("node: %p, neighbor: %d, k: %d\n", node, node->neighbors->size, k);

		for (j = 0; j < node->neighbors->size; j++) {
			scf_graph_node_t* neighbor = node->neighbors->data[j];

			if (neighbor->color > 0) {
				int ret = _x64_color_del(colors2, neighbor->color);
				if (ret < 0)
					goto error;
			}

			scf_logd("node: %p, neighbor: %p, color: %ld\n", node, neighbor, neighbor->color);

			if (0 != scf_vector_add(neighbor->neighbors, node))
				goto error;
		}

		assert(colors2->size >= 0);

		if (0 == node->color) {
			node->color = _x64_color_select(node, colors2);
			if (0 == node->color) {
				node->color = -1;
				scf_logw("colors2->size: %d\n", colors2->size);
			}
		}

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

static int _x64_kcolor_find_not_neighbor(scf_graph_t* graph, int k, scf_graph_node_t** pp0, scf_graph_node_t** pp1)
{
	assert(graph->nodes->size >= k);

	int i;
	for (i = 0; i < graph->nodes->size; i++) {

		scf_graph_node_t* node0 = graph->nodes->data[i];
		x64_rcg_node_t*   rn    = node0->data;

		if (node0->neighbors->size > k) {
			continue;
		}

		if (rn->reg) {
			assert(node0->color > 0);
			continue;
		}

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

static scf_graph_node_t* _x64_max_neighbors(scf_graph_t* graph)
{
	scf_graph_node_t* node_max = NULL;
	int max = 0;
	int i;
	for (i = 0; i < graph->nodes->size; i++) {
		scf_graph_node_t* node = graph->nodes->data[i];
		x64_rcg_node_t*   rn   = node->data;

		if (rn->reg) {
			assert(node->color > 0);
			continue;
		}

		if (!node_max || max < node->neighbors->size) {
			node_max = node;
			max      = node->neighbors->size;
		}
	}

	x64_rcg_node_t*   rn   = node_max->data;

	if (rn->dag_node->var->w)
		scf_logi("max_neighbors: %d, %s\n", max, rn->dag_node->var->w->text->data);
	else
		scf_logi("max_neighbors: %d, v_%p\n", max, rn->dag_node->var);

	return node_max;
}

static int _x64_kcolor_process_conflict(scf_graph_t* graph)
{
	int i;
	int j;
	int ret;

	for (i = 0; i < graph->nodes->size - 1; i++) {

		scf_graph_node_t* gn0 = graph->nodes->data[i];
		x64_rcg_node_t*   rn0 = gn0->data;

		if (0 == gn0->color || !rn0->dag_node)
			continue;

		for (j = i + 1; j < graph->nodes->size; j++) {

			scf_graph_node_t* gn1 = graph->nodes->data[j];
			x64_rcg_node_t*   rn1 = gn1->data;

			if (0 == gn1->color || !rn1->dag_node)
				continue;

			if (!X64_COLOR_CONFLICT(gn0->color, gn1->color))
				continue;

			int nb_parents0 = rn0->dag_node->parents ? rn0->dag_node->parents->size : 0;
			int nb_parents1 = rn1->dag_node->parents ? rn1->dag_node->parents->size : 0;

			if (nb_parents0 > nb_parents1) {
				rn1->reg    = NULL;
				rn1->OpCode = NULL;
				gn1->color  = 0;
			} else {
				rn0->reg    = NULL;
				rn0->OpCode = NULL;
				gn0->color  = 0;
			}
		}
	}

	return 0;
}

static int _x64_graph_kcolor(scf_graph_t* graph, int k, scf_vector_t* colors)
{
	int ret = -1;
	scf_vector_t* colors2 = NULL;

	scf_vector_t* deleted_nodes = scf_vector_alloc();
	if (!deleted_nodes)
		return -ENOMEM;

	//scf_logi("graph->nodes->size: %d, k: %d\n", graph->nodes->size, k);

	ret = _x64_kcolor_delete(graph, k, deleted_nodes);
	if (ret < 0)
		goto error;

	if (0 == _x64_kcolor_check(graph)) {
	//	scf_logi("graph->nodes->size: %d\n", graph->nodes->size);

		ret = _x64_kcolor_fill(graph, k, colors, deleted_nodes);
		if (ret < 0)
			goto error;

		scf_vector_free(deleted_nodes);
		deleted_nodes = NULL;

	//	scf_logi("graph->nodes->size: %d\n", graph->nodes->size);
		return 0;
	}

	assert(graph->nodes->size > 0);
	assert(graph->nodes->size >= k);

	scf_graph_node_t* node0 = NULL;
	scf_graph_node_t* node1 = NULL;

	if (0 == _x64_kcolor_find_not_neighbor(graph, k, &node0, &node1)) {
		assert(node0);
		assert(node1);

		x64_rcg_node_t* rn0 = node0->data;
		x64_rcg_node_t* rn1 = node1->data;

		assert(!colors2);
		colors2 = scf_vector_clone(colors);
		if (!colors2) {
			ret = -ENOMEM;
			goto error;
		}

		if (rn0->dag_node->var->size > rn1->dag_node->var->size) {

			node0->color   = _x64_color_select(node0, colors2);

			intptr_t type  = X64_COLOR_TYPE(node0->color);
			intptr_t id    = X64_COLOR_ID(node0->color);
			intptr_t mask  = (1 << rn1->dag_node->var->size) - 1; 
			node1->color = X64_COLOR(type, id, mask);
		} else {
			node1->color   = _x64_color_select(node1, colors2);

			intptr_t type  = X64_COLOR_TYPE(node0->color);
			intptr_t id    = X64_COLOR_ID(node0->color);
			intptr_t mask  = (1 << rn0->dag_node->var->size) - 1; 
			node0->color   = X64_COLOR(type, id, mask);
		}

		ret = scf_graph_delete_node(graph, node0);
		if (ret < 0)
			goto error;

		ret = scf_graph_delete_node(graph, node1);
		if (ret < 0)
			goto error;

		ret = scf_vector_del(colors2, (void*)node0->color);
		if (ret < 0)
			goto error;
		assert(!scf_vector_find(colors2, (void*)node1->color));

		ret = scf_x64_graph_kcolor(graph, k - 1, colors2);
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
		scf_graph_node_t* node_max = _x64_max_neighbors(graph);
		assert(node_max);

		ret = scf_graph_delete_node(graph, node_max);
		if (ret < 0)
			goto error;
		node_max->color = -1;

		ret = scf_x64_graph_kcolor(graph, k, colors);
		if (ret < 0)
			goto error;

		ret = scf_graph_add_node(graph, node_max);
		if (ret < 0)
			goto error;
	}

	ret = _x64_kcolor_fill(graph, k, colors, deleted_nodes);
	if (ret < 0)
		goto error;

	scf_vector_free(deleted_nodes);
	deleted_nodes = NULL;

//	scf_logi("graph->nodes->size: %d\n", graph->nodes->size);
	return 0;

error:
	if (colors2)
		scf_vector_free(colors2);

	scf_vector_free(deleted_nodes);
	return ret;
}

int scf_x64_graph_kcolor(scf_graph_t* graph, int k, scf_vector_t* colors)
{
	if (!graph || !colors || 0 == colors->size)
		return -EINVAL;

	_x64_kcolor_process_conflict(graph);

	return _x64_graph_kcolor(graph, k, colors);
}


