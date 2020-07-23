#ifndef SCF_BASIC_BLOCK_H
#define SCF_BASIC_BLOCK_H

#include"scf_list.h"
#include"scf_vector.h"
#include"scf_graph.h"

typedef struct scf_basic_block_s  scf_basic_block_t;

struct scf_basic_block_s
{
	scf_list_t      list; // for function's basic block list

	scf_list_t      dag_list_head;

	scf_list_t      load_list_head;
	scf_list_t      code_list_head;
	scf_list_t      save_list_head;

	scf_vector_t*   var_dag_nodes;

	scf_vector_t*   prevs; // prev basic blocks
	scf_vector_t*   nexts; // next basic blocks

	int             code_bytes;
	int             index;

	uint32_t        jmp_flag:1;
	uint32_t        call_flag:1;
	uint32_t        ret_flag:1;
};


scf_basic_block_t*  scf_basic_block_alloc();

void                scf_basic_block_free(scf_basic_block_t* bb);

void                scf_basic_block_print(scf_basic_block_t* bb, scf_list_t* sentinel);
void                scf_basic_block_print_list(scf_list_t* h);

int                 scf_basic_block_active_vars(scf_basic_block_t* bb);

int                 scf_basic_block_kcolor_vars(scf_basic_block_t* bb, scf_vector_t* colors);

#endif

