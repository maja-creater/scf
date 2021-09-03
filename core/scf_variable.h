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

	int					size;
	int                 data_size;

	int					offset;
	int					bp_offset;  // offset based on RBP / EBP register
	int					sp_offset;  // offset based on RSP / ESP register
	int					ds_offset;  // offset in data section
	void*               rabi;

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

	scf_string_t*       signature;

	uint32_t            const_literal_flag:1;
	uint32_t            const_flag  :1;
	uint32_t            static_flag :1;
	uint32_t            extern_flag :1;
	uint32_t            alloc_flag  :1;
	uint32_t            extra_flag  :1;

	uint32_t            tmp_flag    :1;
	uint32_t            local_flag  :1;
	uint32_t            global_flag :1;
	uint32_t            member_flag :1;

	uint32_t            arg_flag    :1;
	uint32_t            auto_gc_flag:1;
};

struct scf_index_s
{
	scf_variable_t*     member;
	int                 index;
};

struct scf_member_s
{
	scf_variable_t*     base;
	scf_vector_t*       indexes;
};

scf_member_t*   scf_member_alloc (scf_variable_t* base);
void            scf_member_free  (scf_member_t*   m);
int             scf_member_offset(scf_member_t*   m);

int             scf_member_add_index(scf_member_t* m, scf_variable_t* member, int index);

scf_variable_t*	scf_variable_alloc(scf_lex_word_t* w, scf_type_t* t);
scf_variable_t*	scf_variable_clone(scf_variable_t* var);
scf_variable_t*	scf_variable_ref(scf_variable_t* var);
void 			scf_variable_free(scf_variable_t* var);

void 			scf_variable_print(scf_variable_t* var);

void 			scf_variable_add_array_dimention(scf_variable_t* var, int dimention_size);

void 			scf_variable_alloc_space(scf_variable_t* var);

void 			scf_variable_set_array_member(scf_variable_t* array, int index, scf_variable_t* member);
void 			scf_variable_get_array_member(scf_variable_t* array, int index, scf_variable_t* member);

int             scf_variable_same_type(scf_variable_t* v0, scf_variable_t* v1);
int             scf_variable_type_like(scf_variable_t* v0, scf_variable_t* v1);

void            scf_variable_sign_extend(scf_variable_t* v, int bytes);
void            scf_variable_zero_extend(scf_variable_t* v, int bytes);

void            scf_variable_extend_std(scf_variable_t* v, scf_variable_t* std);

void            scf_variable_extend_bytes(scf_variable_t* v, int bytes);

int             scf_variable_size(scf_variable_t* v);

static inline int scf_variable_const(scf_variable_t* v)
{
	if (SCF_FUNCTION_PTR == v->type)
		return v->const_literal_flag;

	if (v->nb_pointers + v->nb_dimentions > 0)
		return v->const_literal_flag;

	return v->const_flag && 0 == v->nb_pointers && 0 == v->nb_dimentions;
}

static inline int scf_variable_const_string(scf_variable_t* v)
{
	return SCF_VAR_CHAR == v->type
		&& v->const_flag
		&& v->const_literal_flag
		&& 1 == v->nb_pointers
		&& 0 == v->nb_dimentions;
}

static inline int scf_variable_float(scf_variable_t* v)
{
	return scf_type_is_float(v->type) && 0 == v->nb_pointers && 0 == v->nb_dimentions;
}

static inline int scf_variable_interger(scf_variable_t* v)
{
	return scf_type_is_integer(v->type) || v->nb_pointers > 0 || v->nb_dimentions > 0;
}

static inline int scf_variable_signed(scf_variable_t* v)
{
	return scf_type_is_signed(v->type) && 0 == v->nb_pointers && 0 == v->nb_dimentions;
}

static inline int scf_variable_unsigned(scf_variable_t* v)
{
	return scf_type_is_unsigned(v->type) || v->nb_pointers > 0 || v->nb_dimentions > 0;
}

static inline int scf_variable_nb_pointers(scf_variable_t* v)
{
	return v->nb_pointers + v->nb_dimentions;
}

static inline int scf_variable_is_struct(scf_variable_t* v)
{
	return v->type >= SCF_STRUCT && 0 == v->nb_pointers && 0 == v->nb_dimentions;
}

static inline int scf_variable_is_struct_pointer(scf_variable_t* v)
{
	return v->type >= SCF_STRUCT && 1 == v->nb_pointers && 0 == v->nb_dimentions;
}

static inline int scf_variable_is_array(scf_variable_t* v)
{
	return v->nb_dimentions > 0;
}

static inline int scf_variable_may_malloced(scf_variable_t* v)
{
	if (v->nb_dimentions > 0)
		return 0;

	if (SCF_FUNCTION_PTR == v->type) {
		if (v->nb_pointers > 1)
			return 1;
		return 0;
	}

	return v->nb_pointers > 0;
}

#endif

