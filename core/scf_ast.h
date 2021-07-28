#ifndef SCF_AST_H
#define SCF_AST_H

#include"scf_type.h"
#include"scf_variable.h"
#include"scf_node.h"
#include"scf_expr.h"
#include"scf_scope.h"
#include"scf_block.h"
#include"scf_function.h"

#define SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag_, nb_pointers_, func_ptr_) \
	({\
		uint32_t        tmp_const_flag  = (t)->node.const_flag;\
		int             tmp_nb_pointers = (t)->nb_pointers;\
		scf_function_t* tmp_func_ptr    = (t)->func_ptr;\
		\
		(t)->node.const_flag = (const_flag_);\
		(t)->nb_pointers     = (nb_pointers_);\
		(t)->func_ptr        = (func_ptr_);\
		\
		scf_variable_t* var  = scf_variable_alloc((w), (t));\
		\
		(t)->node.const_flag = tmp_const_flag;\
		(t)->nb_pointers     = tmp_nb_pointers;\
		(t)->func_ptr        = tmp_func_ptr;\
		var;\
	 })

typedef struct scf_ast_rela_s   scf_ast_rela_t;
typedef struct scf_ast_s		scf_ast_t;

struct scf_ast_rela_s
{
	scf_member_t*       ref; // ref should point to obj's address
	scf_member_t*       obj;
};

struct scf_ast_s
{
	scf_block_t*        root_block;
	scf_block_t*        current_block;

	int					nb_structs;
	int					nb_functions;

	scf_vector_t*       global_consts;
	scf_vector_t*       global_relas;
};

int scf_expr_calculate(scf_ast_t* ast, scf_expr_t* expr, scf_variable_t** pret);

int scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data);

scf_string_t* scf_variable_type_name(scf_ast_t* ast, scf_variable_t* v);

static inline void scf_ast_push_block(scf_ast_t* ast, scf_block_t* b)
{
	scf_node_add_child((scf_node_t*)ast->current_block, (scf_node_t*)b);
	ast->current_block = b;
}

static inline void scf_ast_pop_block(scf_ast_t* ast)
{
	ast->current_block = (scf_block_t*)(ast->current_block->node.parent);
}

static inline scf_block_t* scf_ast_parent_block(scf_ast_t* ast)
{
	scf_node_t* parent = ast->current_block->node.parent;

	while (parent && parent->type != SCF_OP_BLOCK && parent->type != SCF_FUNCTION)
		parent = parent->parent;

	return (scf_block_t*)parent;
}

static inline scf_scope_t* scf_ast_current_scope(scf_ast_t* ast)
{
	return ast->current_block->scope;
}

int scf_ast_find_global_function (scf_function_t** pf, scf_ast_t* ast, char* name);
int scf_ast_find_global_variable (scf_variable_t** pv, scf_ast_t* ast, char* name);
int scf_ast_find_global_type     (scf_type_t**     pt, scf_ast_t* ast, char* name);
int scf_ast_find_global_type_type(scf_type_t**     pt, scf_ast_t* ast, int   type);

int scf_ast_find_function (scf_function_t** pf, scf_ast_t* ast, char* name);
int scf_ast_find_variable (scf_variable_t** pv, scf_ast_t* ast, char* name);
int scf_ast_find_type     (scf_type_t**     pt, scf_ast_t* ast, char* name);
int scf_ast_find_type_type(scf_type_t**     pt, scf_ast_t* ast, int   type);

int scf_operator_function_call(scf_ast_t* ast, scf_function_t* f, const int argc, const scf_variable_t** argv, scf_variable_t** pret, scf_list_t* _3ac_list_head);

int	scf_ast_open(scf_ast_t** past);
int scf_ast_close(scf_ast_t* ast);

int scf_ast_add_base_type(scf_ast_t* ast, scf_base_type_t* base_type);

int scf_ast_add_file_block(scf_ast_t* ast, const char* path);

#endif

