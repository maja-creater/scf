#ifndef SCF_3AC_H
#define SCF_3AC_H

#include"scf_node.h"
#include"scf_dag.h"
#include"scf_graph.h"
#include"scf_basic_block.h"

typedef struct scf_3ac_operator_s	scf_3ac_operator_t;
typedef struct scf_3ac_operand_s	scf_3ac_operand_t;

struct scf_3ac_operator_s {
	int							type;
	const char*					name;
};

struct scf_3ac_operand_s {

	scf_node_t*         node;      // AST node
	scf_dag_node_t*     dag_node;  // for optimizer and native generator
	scf_3ac_code_t*     code;      // for branch jump
	scf_basic_block_t*  bb;        // dst basic block of jmp

	scf_lex_word_t*     debug_w;
	void*               rabi;
};

struct scf_3ac_code_s {
	scf_list_t			list; // for 3ac code list

	scf_3ac_operator_t*	op; // for 3ac operator

	scf_vector_t*		dsts; // dst operands, maybe only used for function return value
	scf_vector_t*		srcs; // src operands, usually 2

	scf_label_t*		label; // only for 'goto' to find the destination to go
	scf_node_t*		    error; // only for 'error'

	scf_3ac_code_t*     origin;

	scf_basic_block_t*  basic_block;
	uint32_t            basic_block_start:1;
	uint32_t            jmp_dst_flag     :1;

	scf_vector_t*		active_vars;
	scf_vector_t*		dn_status_initeds;

	scf_vector_t*		instructions;
	int                 inst_bytes;
	int                 bb_offset;

	scf_graph_t*        rcg;
};

scf_3ac_operand_t*	scf_3ac_operand_alloc();
void				scf_3ac_operand_free(scf_3ac_operand_t* operand);

scf_3ac_code_t*		scf_3ac_code_alloc();
scf_3ac_code_t*		scf_3ac_code_clone(scf_3ac_code_t* c);
void				scf_3ac_code_free(scf_3ac_code_t* code);
void				scf_3ac_code_print(scf_3ac_code_t* c, scf_list_t* sentinel);

scf_3ac_code_t*     scf_branch_ops_code(int type, scf_label_t* l, scf_node_t* err);

scf_3ac_operator_t*	scf_3ac_find_operator(const int type);

int                 scf_3ac_code_to_dag(scf_3ac_code_t* c, scf_list_t* dag);

scf_3ac_code_t*     scf_3ac_alloc_by_src(int op_type, scf_dag_node_t* src);
scf_3ac_code_t*     scf_3ac_alloc_by_dst(int op_type, scf_dag_node_t* dst);
scf_3ac_code_t*     scf_3ac_code_NN(int op_type, scf_node_t** dsts, int nb_dsts, scf_node_t** srcs, int nb_srcs);

int					scf_3ac_split_basic_blocks(scf_list_t* list_head_3ac, scf_function_t* f);

int                 scf_3ac_code_same(scf_3ac_code_t* c0, scf_3ac_code_t* c1);

#endif

