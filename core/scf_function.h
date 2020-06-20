#ifndef SCF_FUNCTION_H
#define SCF_FUNCTION_H

#include"scf_node.h"

struct scf_function_s {
	scf_node_t        node;

	scf_scope_t*      scope;

	scf_string_t*     name;

	scf_lex_word_t*   w_start;
	scf_lex_word_t*   w_end;

	uint32_t          root_flag:1;
	uint32_t          file_flag:1;

	uint32_t          define_flag:1;

	scf_list_t        list; // for scope

	scf_variable_t*   ret;  // return value

	scf_vector_t*     argv; // args vector

	int               op_type; // overloaded operator type
};

scf_function_t*	scf_function_alloc(scf_lex_word_t* w);
void			scf_function_free(scf_function_t* f);

int             scf_function_same(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_type(scf_function_t* f0, scf_function_t* f1);
int             scf_function_same_argv(scf_vector_t* argv0, scf_vector_t* argv1);

#endif

