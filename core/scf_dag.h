#ifndef SCF_DAG_H
#define SCF_DAG_H

#include"scf_vector.h"
#include"scf_variable.h"
#include"scf_node.h"

typedef struct scf_dag_node_s	scf_dag_node_t;

struct scf_dag_node_s {
	scf_list_t			list; // for dag node list in scope

	int					type;	// node type

	scf_variable_t*		var;

	scf_vector_t*		parents;
	scf_vector_t*		nodes;

	int					active;
	int					color;
};

scf_dag_node_t* scf_dag_node_alloc (int type, scf_variable_t* var);

int				scf_dag_node_add_child (scf_dag_node_t* parent, scf_dag_node_t* child);

void			scf_dag_node_free (scf_dag_node_t* dag_node);

scf_dag_node_t*	scf_list_find_dag_node (scf_list_t* h, const scf_node_t* node);

int				scf_dag_vector_find_leafs (scf_vector_t* roots, scf_vector_t* leafs);

int				scf_dag_root_find_leafs (scf_dag_node_t* root, scf_vector_t* leafs);

int				scf_dag_find_leafs (scf_list_t* h, scf_vector_t* leafs);

int				scf_dag_find_roots (scf_list_t* h, scf_vector_t* roots);

#endif

