#ifndef SCF_VARIABLE_H
#define SCF_VARIABLE_H

#include"scf_core_types.h"
#include"scf_lex_word.h"

struct scf_variable_s {
	scf_list_t			list; // for variable scope

	int					refs; // reference count

	int					type; 	// type
	scf_lex_word_t*		w;		// lex word

	int					nb_pointers; // Multiple pointer count
	scf_function_t*     func_ptr;

	int*				dimentions; // var array every dimention size
	int					nb_dimentions; // total dimentions
	int                 dim_index;
	int					capacity;

	int					size; // data size for struct type variable
	int					offset;
	int					bp_offset;  // offset based on RBP / EBP register

	union {
		int32_t         i;
		uint32_t        u32;
		int64_t         i64;
		uint64_t        u64;
		float           f;
		double			d;
		scf_complex_t   z;    // value for complex
		scf_string_t*	s;
		void*			p; // for pointer or struct data or array
	} data;

	uint32_t            const_literal_flag:1;
	uint32_t            const_flag:1;
	uint32_t            alloc_flag:1;
	uint32_t            local_flag:1;
};

scf_variable_t*	scf_variable_alloc(scf_lex_word_t* w, scf_type_t* t);
scf_variable_t*	scf_variable_clone(scf_variable_t* var);
void 			scf_variable_free(scf_variable_t* var);

void 			scf_variable_print(scf_variable_t* var);

void 			scf_variable_add_array_dimention(scf_variable_t* var, int dimention_size);

void 			scf_variable_alloc_space(scf_variable_t* var);

void 			scf_variable_set_array_member(scf_variable_t* array, int index, scf_variable_t* member);
void 			scf_variable_get_array_member(scf_variable_t* array, int index, scf_variable_t* member);

int             scf_variable_same_type(scf_variable_t* v0, scf_variable_t* v1);

#endif

