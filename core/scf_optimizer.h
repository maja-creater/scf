#ifndef SCF_OPTIMIZER_H
#define SCF_OPTIMIZER_H

#include"scf_ast.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

typedef struct scf_optimizer_s   scf_optimizer_t;

#define SCF_OPTIMIZER_LOCAL  0
#define SCF_OPTIMIZER_GLOBAL 1

struct scf_optimizer_s
{
	const char*  name;

	int        (*optimize)(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions);
	uint32_t     flags;
};

int  bbg_find_entry_exit(scf_bb_group_t* bbg);
void scf_loops_print(scf_vector_t* loops);

int scf_optimize(scf_ast_t* ast, scf_vector_t* functions);

#endif

