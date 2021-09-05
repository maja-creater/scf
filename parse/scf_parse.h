#ifndef SCF_PARSE_H
#define SCF_PARSE_H

#include"scf_lex.h"
#include"scf_ast.h"
#include"scf_dfa.h"
#include"scf_stack.h"
#include"scf_dwarf_def.h"

typedef struct scf_parse_s		scf_parse_t;
typedef struct dfa_parse_data_s dfa_parse_data_t;

#define SCF_SHNDX_TEXT   1
#define SCF_SHNDX_RODATA 2
#define SCF_SHNDX_DATA   3

#define SCF_SHNDX_DEBUG_ABBREV 4
#define SCF_SHNDX_DEBUG_INFO   5
#define SCF_SHNDX_DEBUG_LINE   6
#define SCF_SHNDX_DEBUG_STR    7

struct scf_parse_s {
	scf_list_t		   word_list_head; // word list head
	scf_list_t		   error_list_head; // error list head

	scf_list_t		   code_list_head; // 3ac code list head

	scf_lex_t*		   lex;

	scf_ast_t*		   ast;

	scf_dfa_t*		   dfa;
	dfa_parse_data_t*  dfa_data;

	scf_vector_t*      symtab;
	scf_vector_t*      global_consts;

	scf_dwarf_debug_t* debug;
};

typedef struct {
	scf_vector_t*    current_index;
	scf_expr_t*      expr;

} dfa_init_expr_t;

typedef struct {
	scf_lex_word_t*      identity;
	scf_lex_word_t*      type_w;
	scf_type_t*          type;

	int                  nb_pointers;
	scf_function_t*      func_ptr;

	uint32_t             const_flag :1;
	uint32_t             extern_flag:1;
	uint32_t             static_flag:1;
	uint32_t             inline_flag:1;

} dfa_identity_t;

struct dfa_parse_data_s {
	void**               module_datas;

	scf_expr_t*          expr;
	int                  expr_local_flag;

	scf_stack_t*         current_identities;
	scf_variable_t*      current_var;
	scf_lex_word_t*      current_var_w;

	int                  nb_sizeofs;
	int                  nb_containers;

	scf_function_t*      current_function;
	int                  argc;

	scf_lex_word_t*      current_async_w;

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

	scf_node_t*          current_va_start;
	scf_node_t*          current_va_arg;
	scf_node_t*          current_va_end;

	uint32_t             const_flag :1;
	uint32_t             extern_flag:1;
	uint32_t             static_flag:1;
	uint32_t             inline_flag:1;

	int              nb_lbs;
	int              nb_rbs;

	int              nb_lss;
	int              nb_rss;

	int              nb_lps;
	int              nb_rps;
};

int	scf_parse_open(scf_parse_t** pparse);
int scf_parse_close(scf_parse_t* parse);

int scf_parse_file(scf_parse_t* parse, const char* path);

int scf_parse_compile(scf_parse_t* parse, const char* out);

#endif

