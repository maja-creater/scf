#include"../scf_graph.h"

int main()
{
	scf_graph_t* g = scf_graph_alloc();

	scf_graph_node_t* n0 = scf_graph_node_alloc();
	scf_graph_node_t* n1 = scf_graph_node_alloc();
	scf_graph_node_t* n2 = scf_graph_node_alloc();
	scf_graph_node_t* n3 = scf_graph_node_alloc();
	scf_graph_node_t* n4 = scf_graph_node_alloc();
	scf_graph_node_t* n5 = scf_graph_node_alloc();

	scf_graph_make_edge(n1, n2);
	scf_graph_make_edge(n2, n3);
	scf_graph_make_edge(n3, n0);

	scf_graph_add_node(g, n0);
	scf_graph_add_node(g, n3);
	scf_graph_add_node(g, n2);
	scf_graph_add_node(g, n1);

	scf_graph_make_edge(n0, n1);
	scf_graph_make_edge(n1, n0);

	int nb_colors = 2;
	scf_vector_t* colors = scf_vector_alloc();
	int i;
	for (i = 0; i < nb_colors; i++)
		scf_vector_add(colors, (void*)(intptr_t)(i + 1));

	scf_graph_kcolor(g, colors);

	scf_graph_node_print(n0);
	scf_graph_node_print(n1);
	scf_graph_node_print(n2);
	scf_graph_node_print(n3);

	return 0;
}
