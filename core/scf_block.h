#ifndef SCF_BLOCK_H
#define SCF_BLOCK_H

#include"scf_node.h"

struct scf_block_s {
	scf_node_t          node;

	scf_scope_t*        scope;

	scf_string_t*       name;

	scf_lex_word_t*     w_start;
	scf_lex_word_t*		w_end;
};


scf_block_t*	scf_block_alloc(scf_lex_word_t* w);

scf_block_t*	scf_block_alloc_cstr(const char* name);

void			scf_block_end(scf_block_t* b, scf_lex_word_t* w);

void			scf_block_free(scf_block_t* b);

scf_type_t*		scf_block_find_type(scf_block_t* b, const char* name);

scf_type_t*		scf_block_find_type_type(scf_block_t* b, const int type);

scf_variable_t*	scf_block_find_variable(scf_block_t* b, const char* name);

scf_function_t*	scf_block_find_function(scf_block_t* b, const char* name);

scf_label_t*    scf_block_find_label(scf_block_t* b, const char* name);

#endif

