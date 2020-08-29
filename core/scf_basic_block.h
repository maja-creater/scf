#ifndef SCF_BASIC_BLOCK_H
#define SCF_BASIC_BLOCK_H

#include"scf_list.h"
#include"scf_vector.h"
#include"scf_graph.h"

typedef struct scf_basic_block_s  scf_basic_block_t;

typedef struct scf_bb_edge_s      scf_bb_edge_t;
typedef struct scf_bb_group_s     scf_bb_group_t;

struct scf_bb_edge_s
{
	scf_basic_block_t*  start;
	scf_basic_block_t*  end;
};

struct scf_bb_group_s
{
	scf_basic_block_t*  entry;
	scf_basic_block_t*  exit;

	scf_basic_block_t*  pre;
	scf_basic_block_t*  post;

	scf_vector_t*       body;

	scf_bb_group_t*     loop_parent;
	scf_vector_t*       loop_childs;
	int                 loop_layers;
};

struct scf_basic_block_s
{
	scf_list_t      list; // for function's basic block list

	scf_list_t      dag_list_head;

	scf_list_t      code_list_head;

	scf_vector_t*   var_dag_nodes;

	scf_vector_t*   prevs; // prev basic blocks
	scf_vector_t*   nexts; // next basic blocks

	scf_vector_t*   dominators; // basic blocks dominate this block
	int             depth_first_order;

	scf_vector_t*   entry_dn_actives;
	scf_vector_t*   exit_dn_actives;
	scf_vector_t*   dn_updateds;
	scf_vector_t*   dn_loads;
	scf_vector_t*   dn_saves;

	int             code_bytes;
	int             index;

	uint32_t        call_flag   :1;
	uint32_t        jmp_flag    :1;
	uint32_t        jcc_flag    :1;
	uint32_t        ret_flag    :1;
	uint32_t        end_flag    :1;
	uint32_t        group_flag  :1;
	uint32_t        visited_flag:1;
};

scf_basic_block_t*  scf_basic_block_alloc();
void                scf_basic_block_free(scf_basic_block_t* bb);

void                scf_bb_group_free(scf_bb_group_t* bbg);

void                scf_basic_block_print(scf_basic_block_t* bb, scf_list_t* sentinel);
void                scf_basic_block_print_list(scf_list_t* h);

int                 scf_basic_block_active_vars(scf_basic_block_t* bb, scf_list_t* dag_list_head);

int                 scf_basic_block_connect(scf_basic_block_t* prev_bb, scf_basic_block_t* next_bb);

#endif

