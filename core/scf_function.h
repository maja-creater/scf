#ifndef SCF_FUNCTION_H
#define SCF_FUNCTION_H

#include"scf_node.h"

struct scf_function_s {
	scf_node_t        node;

	scf_scope_t*      scope;

	scf_string_t*     signature;

	scf_lex_word_t*   w_start;
	scf_lex_word_t*   w_end;

	scf_list_t        list; // for scope

	scf_vector_t*     rets; // return values
	scf_vector_t*     argv; // args   vector

	int               args_int;
	int               args_float;

	int               op_type; // overloaded operator type

	scf_vector_t*     callee_functions;
	scf_vector_t*     caller_functions;

	scf_list_t        basic_block_list_head;
	int               nb_basic_blocks;

	scf_vector_t*     jmps;

	scf_list_t        dag_list_head;

	scf_vector_t*     dfs_tree;
	scf_vector_t*     bb_loops;
	scf_vector_t*     bb_groups;
	int               max_dfo;
	int               max_dfo_reverse;

	scf_vector_t*     text_relas; // re-localtions in .text segment
	scf_vector_t*     data_relas; // re-localtions in .data segment

	scf_vector_t*     init_insts;
	int               init_code_bytes;

	int               local_vars_size;
	int               code_bytes;

	uint32_t          vargs_flag  :1;
	uint32_t          visited_flag:1;
	uint32_t          bp_used_flag:1;

	uint32_t          static_flag:1;
	uint32_t          extern_flag:1;
	uint32_t          inline_flag:1;
	uint32_t          member_flag:1;
};

scf_function_t*	scf_function_alloc(scf_lex_word_t* w);
void			scf_function_free(scf_function_t* f);

int             scf_function_same(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_type(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_argv(scf_vector_t* argv0, scf_vector_t* argv1);
int             scf_function_like_argv(scf_vector_t* argv0, scf_vector_t* argv1);

int             scf_function_signature(scf_function_t* f);

#endif

