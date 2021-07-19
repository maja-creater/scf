#ifndef SCF_TYPE_H
#define SCF_TYPE_H

#include"scf_node.h"

typedef struct {
	int					type;
	const char*			name;
	int					size;
} scf_base_type_t;

typedef struct {
	const char*         name;
	const char*         abbrev;
} scf_type_abbrev_t;

struct scf_type_s {
	// same as block, only used for class type
	scf_node_t          node;

	scf_scope_t*        scope;

	scf_string_t*       name;

	scf_lex_word_t*     w_start;
	scf_lex_word_t*		w_end;

	// list for scope's type_list_head
	scf_list_t			list;

	int					type;
	scf_lex_word_t*		w;

	int					nb_pointers;
//	int					array_capacity;

	scf_function_t*     func_ptr; // only used for function pointer type

	int					size;
	int					offset; // only used for member var of struct or class

	scf_type_t*			parent; // pointed to parent type includes this
};

scf_type_t*		scf_type_alloc(scf_lex_word_t* w, const char* name, int type, int size);
void			scf_type_free(scf_type_t* t);

const char*     scf_type_find_abbrev(const char* name);

#endif

