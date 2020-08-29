#ifndef SCF_FUNCTION_H
#define SCF_FUNCTION_H

#include"scf_node.h"

struct scf_function_s {
	scf_node_t        node;

	scf_scope_t*      scope;

	scf_string_t*     name;

	scf_lex_word_t*   w_start;
	scf_lex_word_t*   w_end;

	scf_list_t        list; // for scope

	scf_variable_t*   ret;  // return value

	scf_vector_t*     argv; // args vector

	int               op_type; // overloaded operator type

	scf_list_t        basic_block_list_head;
	int               nb_basic_blocks;

	scf_vector_t*     jmps;

	scf_list_t        dag_list_head;

	scf_vector_t*     dfs_tree;
	scf_vector_t*     bb_loops;
	int               max_dfo;

	scf_vector_t*     text_relas; // re-localtions in .text segment
	scf_vector_t*     data_relas; // re-localtions in .data segment

	scf_vector_t*     init_insts;

	int               local_vars_size;
	int               code_bytes;
};

scf_function_t*	scf_function_alloc(scf_lex_word_t* w);
void			scf_function_free(scf_function_t* f);

int             scf_function_same(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_type(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_argv(scf_vector_t* argv0, scf_vector_t* argv1);

scf_string_t*   scf_function_signature(scf_function_t* f);

#endif

