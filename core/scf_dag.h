#ifndef SCF_DAG_H
#define SCF_DAG_H

#include"scf_vector.h"
#include"scf_variable.h"
#include"scf_node.h"

typedef struct scf_dag_node_s    scf_dag_node_t;
typedef struct scf_dn_index_s    scf_dn_index_t;
typedef struct scf_dn_status_s   scf_dn_status_t;

enum scf_dn_alias_type
{
	SCF_DN_ALIAS_NULL = 0,

	SCF_DN_ALIAS_VAR,
	SCF_DN_ALIAS_ARRAY,
	SCF_DN_ALIAS_MEMBER,
	SCF_DN_ALIAS_ALLOC,
};

struct scf_dag_node_s {
	scf_list_t			list; // for dag node list in scope

	int					type;	// node type

	scf_variable_t*		var;
	scf_node_t*         node;

	scf_vector_t*		parents;
	scf_vector_t*		childs;

	void*               rabi;
	void*               rabi2;

	intptr_t            color;

	uint32_t            done   :1;

	uint32_t            active :1;
	uint32_t            inited :1;
	uint32_t            updated:1;

	uint32_t            loaded :1;
};

struct scf_dn_index_s {
	scf_variable_t*     member;
	intptr_t            index;

	scf_dag_node_t*     dn;
	scf_dag_node_t*     dn_scale;

	int                 refs;
};

struct scf_dn_status_s {
	scf_dag_node_t*     dag_node;

	scf_vector_t*       dn_indexes;

	scf_dag_node_t*     alias;
	scf_vector_t*       alias_indexes;
	scf_dag_node_t*     dereference;
	int                 alias_type;

	int                 refs;

	intptr_t            color;

	int                 ret_index;

	uint32_t            active :1;
	uint32_t            inited :1;
	uint32_t            updated:1;
	uint32_t            loaded :1;
	uint32_t            ret    :1;
};

scf_dn_index_t*   scf_dn_index_alloc();
scf_dn_index_t*   scf_dn_index_clone(scf_dn_index_t* di);
void              scf_dn_index_free (scf_dn_index_t* di);

int               scf_dn_index_same(const scf_dn_index_t* di0, const scf_dn_index_t* di1);
int               scf_dn_index_like(const scf_dn_index_t* di0, const scf_dn_index_t* di1);

int               scf_dn_status_is_like(const scf_dn_status_t* ds);

scf_dn_status_t*  scf_dn_status_null();
scf_dn_status_t*  scf_dn_status_alloc(scf_dag_node_t*  dn);
scf_dn_status_t*  scf_dn_status_clone(scf_dn_status_t* ds);
scf_dn_status_t*  scf_dn_status_ref  (scf_dn_status_t* ds);

void              scf_dn_status_free (scf_dn_status_t* ds);
void              scf_dn_status_print(scf_dn_status_t* ds);

int               scf_dn_status_copy_dn   (scf_dn_status_t* dst, scf_dn_status_t* src);
int               scf_dn_status_copy_alias(scf_dn_status_t* dst, scf_dn_status_t* src);

scf_dag_node_t*   scf_dag_node_alloc(int type, scf_variable_t* var, const scf_node_t* node);

int               scf_dag_node_add_child (scf_dag_node_t* parent, scf_dag_node_t* child);

int               scf_dag_node_same (scf_dag_node_t* dag_node, const scf_node_t* node);
int               scf_dag_node_like (scf_dag_node_t* dag_node, const scf_node_t* node, scf_list_t* h);
void              scf_dag_node_free (scf_dag_node_t* dag_node);

void              scf_dag_node_free_list(scf_list_t* dag_list_head);

scf_dag_node_t*	  scf_dag_find_node (scf_list_t* h, const scf_node_t* node);
int               scf_dag_get_node  (scf_list_t* h, const scf_node_t* node, scf_dag_node_t** pp);

int				  scf_dag_vector_find_leafs (scf_vector_t* roots, scf_vector_t* leafs);

int				  scf_dag_root_find_leafs (scf_dag_node_t* root, scf_vector_t* leafs);

int				  scf_dag_find_leafs (scf_list_t* h, scf_vector_t* leafs);

int				  scf_dag_find_roots (scf_list_t* h, scf_vector_t* roots);

int               scf_dag_expr_calculate(scf_list_t* h, scf_dag_node_t* node);

int               scf_dag_dn_same(scf_dag_node_t* dn0, scf_dag_node_t* dn1);

int               scf_dn_status_cmp_alias(const void* p0, const void* p1);

int               scf_dn_status_cmp_same_dn_indexes(const void* p0, const void* p1);
int               scf_dn_status_cmp_like_dn_indexes(const void* p0, const void* p1);

int               scf_dn_status_index(scf_dn_status_t* ds, scf_dag_node_t* dn_index, int type);

int               scf_dn_status_alias_index(scf_dn_status_t* ds, scf_dag_node_t* dn_index, int type);

int               scf_ds_for_dn                (scf_dn_status_t** pds, scf_dag_node_t* dn);
int               scf_ds_for_assign_member     (scf_dn_status_t** pds, scf_dag_node_t* dn_base, scf_dag_node_t* dn_member);
int               scf_ds_for_assign_dereference(scf_dn_status_t** pds, scf_dag_node_t* dn);
int               scf_ds_for_assign_array_member(scf_dn_status_t** pds, scf_dag_node_t* dn_base, scf_dag_node_t* dn_index, scf_dag_node_t* dn_scale);

void              scf_dn_status_vector_clear_by_ds(scf_vector_t* vec, scf_dn_status_t* ds);
void              scf_dn_status_vector_clear_by_dn(scf_vector_t* vec, scf_dag_node_t*  dn);

static int scf_dn_through_bb(scf_dag_node_t* dn)
{
	return (dn->var->global_flag || dn->var->local_flag || dn->var->tmp_flag)
		&& !scf_variable_const(dn->var);
}

static int scf_dn_status_cmp(const void* p0, const void* p1)
{
	const scf_dag_node_t*  dn0 = p0;
	const scf_dn_status_t* ds1 = p1;

	if (ds1->dn_indexes)
		return -1;

	return dn0 != ds1->dag_node;
}

static int scf_dn_status_cmp_dereference(const void* p0, const void* p1)
{
	const scf_dag_node_t*  dn0 = p0;
	const scf_dn_status_t* ds1 = p1;

	return dn0 != ds1->dereference;
}

static inline int scf_ds_is_pointer(scf_dn_status_t* ds)
{
	if (!ds->dn_indexes)
		return ds->dag_node->var->nb_pointers > 0;

	scf_dn_index_t* di;

	int n = scf_variable_nb_pointers(ds->dag_node->var);
	int i;

	for (i = ds->dn_indexes->size - 1; i >= 0; i--) {
		di = ds->dn_indexes->data[i];

		if (di->member)
			n = scf_variable_nb_pointers(di->member);
		else
			n--;
	}

	assert(n >= 0);
	return n;
}

static inline int scf_ds_nb_pointers(scf_dn_status_t* ds)
{
	if (!ds->dn_indexes)
		return ds->dag_node->var->nb_pointers;

	scf_dn_index_t* di;

	int n = scf_variable_nb_pointers(ds->dag_node->var);
	int i;

	for (i = ds->dn_indexes->size - 1; i >= 0; i--) {
		di = ds->dn_indexes->data[i];

		if (di->member)
			n = scf_variable_nb_pointers(di->member);
		else
			n--;
	}

	assert(n >= 0);
	return n;
}

#define SCF_DN_STATUS_GET(ds, vec, dn) \
	do { \
		ds = scf_vector_find_cmp(vec, dn, scf_dn_status_cmp); \
		if (!ds) { \
			ds = scf_dn_status_alloc(dn); \
			if (!ds) \
				return -ENOMEM; \
			int ret = scf_vector_add(vec, ds); \
			if (ret < 0) { \
				scf_dn_status_free(ds); \
				return ret; \
			} \
		} \
	} while (0)

#define SCF_DN_STATUS_ALLOC(ds, vec, dn) \
	do { \
		ds = scf_dn_status_alloc(dn); \
		if (!ds) \
			return -ENOMEM; \
		int ret = scf_vector_add(vec, ds); \
		if (ret < 0) { \
			scf_dn_status_free(ds); \
			return ret; \
		} \
	} while (0)

#endif

