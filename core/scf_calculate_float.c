#include"scf_calculate.h"

#define SCF_FLOAT_BINARY_OP(name, op) \
int scf_float_##name(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src0, scf_variable_t* src1) \
{ \
	scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_FLOAT); \
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 0, NULL); \
	if (!r) \
		return -ENOMEM; \
	r->data.f = src0->data.f op src1->data.f; \
	if (pret) \
		*pret = r; \
	return 0; \
}

#define SCF_FLOAT_UNARY_OP(name, op) \
int scf_float_##name(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src0, scf_variable_t* src1) \
{ \
	scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_FLOAT); \
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 0, NULL); \
	if (!r) \
		return -ENOMEM; \
	r->data.f = op src0->data.f; \
	if (pret) \
		*pret = r; \
	return 0; \
}

SCF_FLOAT_BINARY_OP(add, +)
SCF_FLOAT_BINARY_OP(sub, -)
SCF_FLOAT_BINARY_OP(mul, *)
SCF_FLOAT_BINARY_OP(div, /)

SCF_FLOAT_UNARY_OP(neg, -)

SCF_FLOAT_BINARY_OP(gt, >)
SCF_FLOAT_BINARY_OP(lt, <)
