#ifndef SCF_DAG_H
#define SCF_DAG_H

#include"scf_vector.h"
#include"scf_variable.h"
#include"scf_node.h"

typedef struct scf_dag_node_s	scf_dag_node_t;
typedef struct scf_active_var_s scf_active_var_t;

struct scf_dag_node_s {
	scf_list_t			list; // for dag node list in scope

	int					type;	// node type

	scf_variable_t*		var;

	scf_vector_t*		parents;
	scf_vector_t*		childs;

	intptr_t            color;
	int					active;
};

struct scf_active_var_s {
	scf_dag_node_t*		dag_node;
	int					active;
};

scf_active_var_t* scf_active_var_alloc(scf_dag_node_t* dag_node);
void              scf_active_var_free(scf_active_var_t* v);

scf_dag_node_t*   scf_dag_node_alloc (int type, scf_variable_t* var);

int				  scf_dag_node_add_child (scf_dag_node_t* parent, scf_dag_node_t* child);

void			  scf_dag_node_free (scf_dag_node_t* dag_node);

scf_dag_node_t*	  scf_dag_find_node (scf_list_t* h, const scf_node_t* node);
int               scf_dag_get_node  (scf_list_t* h, const scf_node_t* node, scf_dag_node_t** pp);

int				  scf_dag_vector_find_leafs (scf_vector_t* roots, scf_vector_t* leafs);

int				  scf_dag_root_find_leafs (scf_dag_node_t* root, scf_vector_t* leafs);

int				  scf_dag_find_leafs (scf_list_t* h, scf_vector_t* leafs);

int				  scf_dag_find_roots (scf_list_t* h, scf_vector_t* roots);

#endif

