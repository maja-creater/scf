#ifndef SCF_SCOPE_H
#define SCF_SCOPE_H

#include"scf_list.h"
#include"scf_vector.h"
#include"scf_lex_word.h"
#include"scf_core_types.h"

struct scf_scope_s {
	scf_list_t			list; // scope list
	scf_string_t*		name; // scope name

	scf_lex_word_t*		w; // scope name

	scf_vector_t*       vars;	        // vars in this scope, should not have the same name
	scf_list_t			type_list_head; // type list in this scope, not define the same type
	scf_list_t			operator_list_head; // operator list in this scope
	scf_list_t			function_list_head; // function list in this scope
	scf_list_t			label_list_head; // label list in this scope
};

scf_scope_t*    scf_scope_alloc(scf_lex_word_t* w, const char* name);
void            scf_scope_free( scf_scope_t*    scope);

void            scf_scope_push_var(scf_scope_t* scope, scf_variable_t* var);

void            scf_scope_push_type(scf_scope_t* scope, scf_type_t* t);

void            scf_scope_push_operator(scf_scope_t* scope, scf_function_t* op);

void            scf_scope_push_function(scf_scope_t* scope, scf_function_t* f);

scf_type_t*     scf_scope_find_type(scf_scope_t* scope, const char* name);

scf_type_t*     scf_scope_find_type_type(scf_scope_t* scope, const int type);

scf_variable_t* scf_scope_find_variable(scf_scope_t* scope, const char* name);

scf_function_t* scf_scope_find_function(scf_scope_t* scope, const char* name);

scf_function_t*	scf_scope_find_same_function(scf_scope_t* scope, scf_function_t* f0);

scf_function_t*	scf_scope_find_proper_function(scf_scope_t* scope, const char* name, scf_vector_t* argv);

int             scf_scope_find_overloaded_functions(scf_vector_t** pfunctions, scf_scope_t* scope, const int op_type, scf_vector_t* argv);

int             scf_scope_find_like_functions(scf_vector_t** pfunctions, scf_scope_t* scope, const char* name, scf_vector_t* argv);

scf_type_t*     scf_scope_list_find_type(scf_list_t* h, const char* name);

void            scf_scope_push_label(scf_scope_t* scope, scf_label_t* l);
scf_label_t*    scf_scope_find_label(scf_scope_t* scope, const char* name);

#endif

