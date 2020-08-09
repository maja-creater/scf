#ifndef SCF_TYPE_CAST_H
#define SCF_TYPE_CAST_H

#include"scf_ast.h"

typedef struct {
	const char* name;

	int			src_type;
	int			dst_type;

	int			(*func)(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);

} scf_type_cast_t;

int               scf_type_cast_check(scf_ast_t* ast, scf_variable_t* dst, scf_variable_t* src);

scf_type_cast_t*  scf_find_base_type_cast(int src_type, int dst_type);

int scf_find_updated_type(scf_ast_t* ast, scf_variable_t* v0, scf_variable_t* v1);

int scf_cast_to_i8 (scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_i16(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_i32(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_i64(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);

int scf_cast_to_u8 (scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_u16(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_u32(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);
int scf_cast_to_u64(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);

int scf_cast_to_float(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);

int scf_cast_to_double(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src);

#endif

