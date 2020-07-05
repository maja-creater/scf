#ifndef SCF_CALCULATE_H
#define SCF_CALCULATE_H

#include"scf_ast.h"

typedef struct {
	const char* name;

	int			op_type;

	int			src0_type;
	int			src1_type;
	int			ret_type;

	int			(*func)(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src0, scf_variable_t* src1);

} scf_calculate_t;

scf_calculate_t* scf_find_base_calculate(int op_type, int src0_type, int src1_type);

#endif

