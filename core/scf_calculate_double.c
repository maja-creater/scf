#include"scf_calculate.h"

#define SCF_DOUBLE_BINARY_OP(name, op) \
int scf_double_##name(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src0, scf_variable_t* src1) \
{ \
	scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_DOUBLE); \
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 0, NULL); \
	if (!r) \
		return -ENOMEM; \
	r->data.d = src0->data.d op src1->data.d; \
	if (pret) \
		*pret = r; \
	return 0; \
}

#define SCF_DOUBLE_UNARY_OP(name, op) \
int scf_double_##name(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src0, scf_variable_t* src1) \
{ \
	scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_DOUBLE); \
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 0, NULL); \
	if (!r) \
		return -ENOMEM; \
	r->data.d = op src0->data.d; \
	if (pret) \
		*pret = r; \
	return 0; \
}

SCF_DOUBLE_BINARY_OP(add, +)
SCF_DOUBLE_BINARY_OP(sub, -)
SCF_DOUBLE_BINARY_OP(mul, *)
SCF_DOUBLE_BINARY_OP(div, /)

SCF_DOUBLE_UNARY_OP(neg, -)

SCF_DOUBLE_BINARY_OP(gt, >)
SCF_DOUBLE_BINARY_OP(lt, <)
