#ifndef SCF_POINTER_ALIAS_H
#define SCF_POINTER_ALIAS_H

#include"scf_function.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

#define SCF_POINTER_NOT_INIT -1000

int _bb_copy_aliases (scf_basic_block_t* bb, scf_dag_node_t* dn_pointer, scf_dag_node_t* dn_dereference, scf_vector_t* aliases);

int _bb_copy_aliases2(scf_basic_block_t* bb, scf_vector_t* aliases);

int __alias_dereference(scf_vector_t* aliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);

int scf_pointer_alias   (scf_vector_t* aliases, scf_dag_node_t*  dn_alias,   scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);
int scf_pointer_alias_ds(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);

int scf_pointer_alias_ds_leak(scf_vector_t* aliases, scf_dn_status_t* ds_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);

#endif
