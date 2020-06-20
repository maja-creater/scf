#ifndef SCF_OPERATOR_HANDLER_H
#define SCF_OPERATOR_HANDLER_H

#include"scf_ast.h"

typedef struct scf_operator_handler_s	scf_operator_handler_t;

typedef int	(*scf_operator_handler_pt)(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data);

struct scf_operator_handler_s {

	scf_list_t				list;

	int						type;

	int						src0_type;
	int						src1_type;
	int						ret_type;

	scf_operator_handler_pt	func;
};

scf_operator_handler_t* scf_operator_handler_alloc(int type,
		int src0_type, int src1_type, int ret_type,
		scf_operator_handler_pt func);

void scf_operator_handler_free(scf_operator_handler_t* h);

scf_operator_handler_t* scf_find_3ac_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type);

int scf_function_to_3ac(scf_ast_t* ast, scf_function_t* f, scf_list_t* _3ac_list_head);

#endif

