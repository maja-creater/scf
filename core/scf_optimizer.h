#ifndef SCF_OPTIMIZER_H
#define SCF_OPTIMIZER_H

#include"scf_ast.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

typedef struct scf_optimizer_s   scf_optimizer_t;

struct scf_optimizer_s
{
	const char* name;

	int         (*optimize)(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head);
};


int scf_optimize(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head);

#endif

