#ifndef SCF_3AC_H
#define SCF_3AC_H

#include"scf_node.h"
#include"scf_dag.h"

typedef struct scf_3ac_operator_s	scf_3ac_operator_t;
typedef struct scf_3ac_operand_s	scf_3ac_operand_t;
typedef struct scf_3ac_code_s		scf_3ac_code_t;

struct scf_3ac_operator_s {
	int							type;
	const char*					name;
};

struct scf_3ac_operand_s {

	scf_node_t*			node; // for calculate
	scf_dag_node_t*		dag_node; // for calculate
	scf_3ac_code_t*		code; // for branch jump
};

typedef struct {
	scf_dag_node_t*		dag_node;
	int					active;
} scf_3ac_active_var_t;

struct scf_3ac_code_s {
	scf_list_t			list; // for 3ac code list

	scf_3ac_operator_t*	op; // for 3ac operator

	scf_3ac_operand_t*	dst; // dst operand

	scf_vector_t*		srcs; // src operands, usually 2

	scf_label_t*		label; // only for 'goto' to find the destination to go
	scf_node_t*		    error; // only for 'error'

	scf_string_t*       extern_fname; // only for call external function

	int					basic_block_start;
	scf_vector_t*		active_vars;

	scf_vector_t*		instructions;
};

typedef struct {
	scf_list_t			dag_list_head;

	scf_vector_t*		codes; // 3ac codes

	scf_vector_t*		var_dag_nodes;

	scf_vector_t*		prev_bbs;
	scf_vector_t*		next_bbs;

	uint32_t            jmp_flag:1;
	uint32_t            call_flag:1;

} scf_3ac_basic_block_t;

typedef struct {
	scf_list_t			_3ac_list_head;
	scf_list_t			dag_list_head;


	scf_vector_t*		basic_blocks;

} scf_3ac_context_t;

scf_3ac_operand_t*	scf_3ac_operand_alloc();
void				scf_3ac_operand_free(scf_3ac_operand_t* operand);

scf_3ac_code_t*		scf_3ac_code_alloc();
void				scf_3ac_code_free(scf_3ac_code_t* code);

scf_3ac_operator_t*	scf_3ac_find_operator(const int type);

void 				_3ac_code_print(scf_list_t* h, scf_3ac_code_t* c);
void				scf_3ac_code_print(scf_list_t* h);

void 				scf_3ac_code_to_dag(scf_list_t* h, scf_list_t* dag);
void 				scf_dag_to_3ac_code(scf_dag_node_t* dag, scf_list_t* h);

int 				scf_3ac_code_make_register_conflict_graph(scf_3ac_code_t* c, scf_graph_t* graph);
void				scf_3ac_colored_graph_print(scf_graph_t* graph);

scf_3ac_context_t*	scf_3ac_context_alloc();
void				scf_3ac_context_free(scf_3ac_context_t* _3ac_ctx);
int					scf_3ac_filter(scf_3ac_context_t* _3ac_ctx, scf_list_t* _3ac_dst_list, scf_list_t* _3ac_src_list);


#endif

