#ifndef SCF_NODE_H
#define SCF_NODE_H

#include"scf_string.h"
#include"scf_list.h"
#include"scf_vector.h"
#include"scf_graph.h"
#include"scf_lex.h"
#include"scf_variable.h"
#include"scf_operator.h"

#define SCF_OP_ASSOCIATIVITY_LEFT	0
#define SCF_OP_ASSOCIATIVITY_RIGHT	1

struct scf_node_s {
	int					type;	// node type

	scf_node_t*			parent;		// parent node
	int					nb_nodes;	// children nodes count
	scf_node_t**		nodes;		// children nodes

	union {
		scf_variable_t*		var;
		scf_lex_word_t*		w;
		scf_label_t*		label;
	};

	scf_lex_word_t*		debug_w;

	int					priority;
	scf_operator_t*		op;

	scf_variable_t*		result;
	scf_vector_t*       result_nodes;
	scf_node_t*         split_parent; // split from the parent

	uint32_t            root_flag   :1; // set when node is root block
	uint32_t            file_flag   :1; // set when node is a file block
	uint32_t            class_flag  :1; // set when node is a class type
	uint32_t            union_flag  :1; // set when node is a union type
	uint32_t            define_flag :1; // set when node is a function & is defined not only declared
	uint32_t            const_flag  :1; // set when node is a const type
	uint32_t            split_flag  :1; // set when node is a split node of its parent
	uint32_t            _3ac_done   :1; // set when node's 3ac code is made

	uint32_t            semi_flag   :1; // set when followed by a ';'
};

struct scf_label_s {
	scf_list_t			list; // for variable scope

	int					refs; // reference count

	int					type;

	scf_lex_word_t*		w;

	scf_node_t*			node; // the labeled node
};

scf_node_t*		scf_node_alloc(scf_lex_word_t* w, int type, scf_variable_t* var);
scf_node_t*		scf_node_alloc_label(scf_label_t* l);

int				scf_node_add_child(scf_node_t* parent, scf_node_t* child);

void			scf_node_free(scf_node_t* node);
void			scf_node_free_data(scf_node_t* node);
void			scf_node_move_data(scf_node_t* dst, scf_node_t* src);

void            scf_node_print(scf_node_t* node);

scf_variable_t* _scf_operand_get(const scf_node_t* node);
scf_function_t* _scf_function_get(scf_node_t* node);

typedef int     (*scf_node_find_pt)(scf_node_t* node, void* arg, scf_vector_t* results);
int             scf_node_search_bfs(scf_node_t* root, void* arg, scf_vector_t* results, int max, scf_node_find_pt find);

scf_label_t*	scf_label_alloc(scf_lex_word_t* w);
void			scf_label_free(scf_label_t* l);

#endif

