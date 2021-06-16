#include"scf_ast.h"

int	scf_ast_open(scf_ast_t** past)
{
	assert(past);

	scf_ast_t* ast = calloc(1, sizeof(scf_ast_t));
	assert(ast);

	ast->root_block = scf_block_alloc_cstr("global");
	ast->root_block->node.root_flag = 1;

	ast->global_consts = scf_vector_alloc();
	if (!ast->global_consts)
		return -ENOMEM;

	ast->global_relas  = scf_vector_alloc();
	if (!ast->global_relas)
		return -ENOMEM;

	*past = ast;
	return 0;
}

int scf_ast_close(scf_ast_t* ast)
{
	if (ast) {
		free(ast);
		ast = NULL;
	}
	return 0;
}

int scf_ast_add_base_type(scf_ast_t* ast, scf_base_type_t* base_type)
{
	if (!ast || !base_type) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_type_t* t = scf_type_alloc(NULL, base_type->name, base_type->type, base_type->size);
	if (!t) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_scope_push_type(ast->root_block->scope, t);
	return 0;
}

int scf_ast_add_file_block(scf_ast_t* ast, const char* path)
{
	if (!ast || !path) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_block_t* file_block = scf_block_alloc_cstr(path);
	if (!file_block) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	file_block->node.file_flag = 1;

	scf_node_add_child((scf_node_t*)ast->root_block, (scf_node_t*)file_block);
	ast->current_block = file_block;
	printf("%s(),%d, file_block: %p\n", __func__, __LINE__, file_block);

	return 0;
}

scf_string_t* scf_variable_type_name(scf_ast_t* ast, scf_variable_t* v)
{
	scf_type_t*   t = scf_ast_find_type_type(ast, v->type);

	scf_string_t* s = scf_string_clone(t->name);

	int i;
	for (i = 0; i < v->nb_pointers; i++)
		scf_string_cat_cstr_len(s, "*", 1);

	for (i = 0; i < v->nb_dimentions; i++) {
		char str[256];
		snprintf(str, sizeof(str) - 1, "[%d]", v->dimentions[i]);

		scf_string_cat_cstr(s, str);
	}

	return s;
}
