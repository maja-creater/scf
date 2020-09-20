#ifndef SCF_DAG_H
#define SCF_DAG_H

#include"scf_vector.h"
#include"scf_variable.h"
#include"scf_node.h"

typedef struct scf_dag_node_s	scf_dag_node_t;
typedef struct scf_active_var_s scf_active_var_t;

enum scf_dn_alias_type
{
	SCF_DN_ALIAS_NONE = 0,

	SCF_DN_ALIAS_VAR,
	SCF_DN_ALIAS_ARRAY,
	SCF_DN_ALIAS_MEMBER,
};

struct scf_dag_node_s {
	scf_list_t			list; // for dag node list in scope

	int					type;	// node type

	scf_variable_t*		var;

	scf_vector_t*		parents;
	scf_vector_t*		childs;

	scf_dag_node_t*     alias;
	intptr_t            index;
	scf_variable_t*     member;
	int                 alias_type;

	void*               rabi;
	void*               rabi2;

	intptr_t            color;

	uint32_t            active :1;
	uint32_t            inited :1;
	uint32_t            updated:1;

	uint32_t            loaded :1;
};

struct scf_active_var_s {
	scf_dag_node_t*     dag_node;

	scf_dag_node_t*     alias;
	intptr_t            index;
	scf_variable_t*     member;
	int                 alias_type;
	scf_dag_node_t*     dereference;

	intptr_t            color;

	uint32_t            active :1;
	uint32_t            inited :1;
	uint32_t            updated:1;
};

scf_active_var_t* scf_active_var_alloc(scf_dag_node_t* dag_node);
void              scf_active_var_free(scf_active_var_t* v);

scf_dag_node_t*   scf_dag_node_alloc (int type, scf_variable_t* var);

int				  scf_dag_node_add_child (scf_dag_node_t* parent, scf_dag_node_t* child);

void			  scf_dag_node_free (scf_dag_node_t* dag_node);

void              scf_dag_node_free_list(scf_list_t* dag_list_head);

scf_dag_node_t*	  scf_dag_find_node (scf_list_t* h, const scf_node_t* node);
int               scf_dag_get_node  (scf_list_t* h, const scf_node_t* node, scf_dag_node_t** pp);

int				  scf_dag_vector_find_leafs (scf_vector_t* roots, scf_vector_t* leafs);

int				  scf_dag_root_find_leafs (scf_dag_node_t* root, scf_vector_t* leafs);

int				  scf_dag_find_leafs (scf_list_t* h, scf_vector_t* leafs);

int				  scf_dag_find_roots (scf_list_t* h, scf_vector_t* roots);

scf_dag_node_t*   scf_dag_find_dn(scf_list_t* h, const scf_dag_node_t* dn0);
int               scf_dag_get_dn (scf_list_t* h, const scf_dag_node_t* dn0, scf_dag_node_t** pp);

int               scf_dag_pointer_alias(scf_active_var_t* v, scf_dag_node_t* dn, scf_3ac_code_t* c);

int               scf_dag_expr_calculate(scf_list_t* h, scf_dag_node_t* node);

int               scf_dag_dn_same(scf_dag_node_t* dn0, scf_dag_node_t* dn1);

static int scf_dn_status_cmp(const void* p0, const void* p1)
{
	const scf_dag_node_t*   dn0 = p0;
	const scf_active_var_t* v1  = p1;

	return dn0 != v1->dag_node;
}

static int scf_dn_status_cmp_dereference(const void* p0, const void* p1)
{
	const scf_dag_node_t*   dn0 = p0;
	const scf_active_var_t* v1  = p1;

	return dn0 != v1->dereference;
}

static int scf_dn_status_cmp_alias(const void* p0, const void* p1)
{
	const scf_active_var_t* v0 = p0;
	const scf_active_var_t* v1 = p1;

	if (v0->alias_type != v1->alias_type)
		return -1;

	if (v0->alias != v1->alias)
		return -1;

	switch (v0->alias_type) {

		case SCF_DN_ALIAS_VAR:
			return 0;
			break;

		case SCF_DN_ALIAS_ARRAY:
			if (v0->index == v1->index)
				return 0;
			break;

		case SCF_DN_ALIAS_MEMBER:
			if (v0->member == v1->member)
				return 0;
			break;
		default:
			break;
	};

	return -1;
}

#define SCF_DN_STATUS_GET(ds, vec, dn) \
	do { \
		ds = scf_vector_find_cmp(vec, dn, scf_dn_status_cmp); \
		if (!ds) { \
			ds = scf_active_var_alloc(dn); \
			if (!ds) \
				return -ENOMEM; \
			int ret = scf_vector_add(vec, ds); \
			if (ret < 0) { \
				scf_active_var_free(ds); \
				return ret; \
			} \
		} \
	} while (0)

#endif

