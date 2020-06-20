#ifndef SCF_PARSE_H
#define SCF_PARSE_H

#include"scf_lex.h"
#include"scf_ast.h"
#include"scf_dfa.h"

typedef struct scf_parse_s		scf_parse_t;
typedef struct dfa_parse_data_s dfa_parse_data_t;

struct scf_parse_s {
	scf_list_t		   word_list_head; // word list head
	scf_list_t		   error_list_head; // error list head

	scf_list_t		   code_list_head; // 3ac code list head

	scf_lex_t*		   lex;

	scf_ast_t*		   ast;

	scf_dfa_t*		   dfa;
	dfa_parse_data_t*  dfa_data;
};

typedef struct {
	scf_vector_t*    current_index;
	scf_expr_t*      expr;

} dfa_init_expr_t;

struct dfa_parse_data_s {
	void**               module_datas;

	scf_expr_t*          expr;
	int                  expr_local_flag;

	scf_type_t*          current_type;
	scf_lex_word_t*      current_type_w;
	scf_lex_word_t*      current_identity;
	scf_variable_t*      current_var;

	int                  nb_sizeofs;

	scf_function_t*      current_function;
	int                  argc;

	// every dimention has a vector,
	// every vector has init exprs for this dimention
	scf_vector_t*        init_exprs;
	int                  current_dim;
	scf_vector_t*        current_index;

	scf_type_t*	         root_struct;
	scf_type_t*	         current_struct;

	scf_node_t*          current_node;

	scf_node_t*          current_while;

	scf_node_t*          current_for;
	scf_vector_t*        for_exprs;

	scf_node_t*          current_return;
	scf_node_t*          current_goto;

	int                  nb_pointers;
	int                  const_flag;
	scf_function_t*      func_ptr;

	int              nb_lbs;
	int              nb_rbs;

	int              nb_lss;
	int              nb_rss;

	int              nb_lps;
	int              nb_rps;

};

int	scf_parse_open(scf_parse_t** pparse, const char* path);
int scf_parse_close(scf_parse_t* parse);

int scf_parse_parse(scf_parse_t* parse);

int scf_parse_run(scf_parse_t* parse, const char* fname, const int argc, const scf_variable_t** argv, scf_variable_t** pret);

#endif

