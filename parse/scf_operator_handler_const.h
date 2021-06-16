#ifndef SCF_OPERATOR_HANDLER_CONST_H
#define SCF_OPERATOR_HANDLER_CONST_H

#include"scf_operator_handler.h"

scf_operator_handler_t* scf_find_const_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type);

int scf_function_const_opt(scf_ast_t* ast, scf_function_t* f);

int scf_const_opt(scf_ast_t* ast);

#endif

