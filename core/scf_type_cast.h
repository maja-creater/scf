#ifndef SCF_TYPE_CAST_H
#define SCF_TYPE_CAST_H

#include"scf_ast.h"

typedef struct {

	int			src_type;
	int			dst_type;

	int			(*handler)(scf_variable_t* dst, scf_variable_t* src);

} scf_type_cast_t;

int scf_type_cast_check(scf_ast_t* ast, scf_variable_t* dst, scf_variable_t* src);

#endif

