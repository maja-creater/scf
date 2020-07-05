#ifndef SCF_GRAPH_H
#define SCF_GRAPH_H

#include"scf_vector.h"

typedef struct {
	scf_vector_t*	neighbors;

	intptr_t        color;
	void*			data;

} scf_graph_node_t;

typedef struct {
	scf_vector_t*		nodes;
} scf_graph_t;


scf_graph_t*		scf_graph_alloc();
void				scf_graph_free(scf_graph_t* graph);

scf_graph_node_t*	scf_graph_node_alloc();
void				scf_graph_node_free(scf_graph_node_t* node);
void				scf_graph_node_print(scf_graph_node_t* node);

int                 scf_graph_make_edge(scf_graph_node_t* node1, scf_graph_node_t* node2);

int                 scf_graph_delete_node(scf_graph_t* graph, scf_graph_node_t* node);
int                 scf_graph_add_node(scf_graph_t* graph, scf_graph_node_t* node);

// color = 0, not colored
// color > 0, color of node
// color < 0, node should be saved to memory
int                 scf_graph_kcolor(scf_graph_t* graph, scf_vector_t* colors);

#endif

