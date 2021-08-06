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
	scf_type_t* t = NULL;

	int ret = scf_ast_find_type_type(&t, ast, v->type);
	assert(0 == ret);
	assert(t);

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

static int _find_function_by_name(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (node->class_flag)
		return 1;

	if (SCF_FUNCTION == node->type) {

		scf_function_t* f = (scf_function_t*)node;

		assert(!f->member_flag);

		if (f->static_flag)
			return 0;

		if (!strcmp(f->node.w->text->data, arg)) {

			int ret = scf_vector_add(vec, f);
			if (ret < 0)
				return ret;
		}
		return 1;
	}

	return 0;
}

static int _find_type_by_name(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (SCF_FUNCTION == node->type)
		return 1;

	if (node->type >= SCF_STRUCT && node->class_flag) {

		scf_type_t* t = (scf_type_t*)node;

		if (!strcmp(t->name->data, arg)) {

			int ret = scf_vector_add(vec, t);
			if (ret < 0)
				return ret;
		}

		return 1;
	}

	return 0;
}

static int _find_type_by_type(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (SCF_FUNCTION == node->type)
		return 1;

	if (node->type >= SCF_STRUCT && (node->class_flag || node->union_flag)) {

		scf_type_t* t = (scf_type_t*)node;

		if (t->type == (intptr_t)arg) {

			int ret = scf_vector_add(vec, t);
			if (ret < 0)
				return ret;
		}

		if (node->union_flag)
			return 1;
	}

	return 0;
}

static int _find_var_by_name(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (SCF_FUNCTION == node->type)
		return 1;

	if (node->class_flag)
		return 1;

	if (SCF_OP_BLOCK == node->type) {

		scf_block_t* b = (scf_block_t*)node;

		if (!b->scope)
			return 0;

		scf_variable_t* v = scf_scope_find_variable(b->scope, arg);
		if (!v)
			return 0;

		assert(!v->local_flag && !v->member_flag);

		if (v->static_flag)
			return 0;

		return scf_vector_add(vec, v);
	}
	return 0;
}

int scf_ast_find_global_function(scf_function_t** pf, scf_ast_t* ast, char* fname)
{
	scf_vector_t* vec = scf_vector_alloc();
	if (!vec)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)ast->root_block, fname, vec, -1, _find_function_by_name);
	if (ret < 0) {
		scf_vector_free(vec);
		return ret;
	}

	if (0 == vec->size) {
		*pf = NULL;
		scf_vector_free(vec);
		return 0;
	}

	scf_function_t* f2 = vec->data[0];
	scf_function_t* f;
	int n = 0;
	int i;
	for (i = 0; i < vec->size; i++) {
		f  =        vec->data[i];

		if (f->node.define_flag) {
			f2 = f;
			n++;
		}
	}

	if (n > 1) {
		for (i = 0; i < vec->size; i++) {
			f  =        vec->data[i];

			if (f->node.define_flag)
				scf_loge("multi-define: '%s' in file: %s, line: %d\n", fname, f->node.w->file->data, f->node.w->line);
		}

		scf_vector_free(vec);
		return -1;
	}

	*pf = f2;

	scf_vector_free(vec);
	return 0;
}

int scf_ast_find_global_variable(scf_variable_t** pv, scf_ast_t* ast, char* name)
{
	scf_vector_t* vec = scf_vector_alloc();
	if (!vec)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)ast->root_block, name, vec, -1, _find_var_by_name);
	if (ret < 0) {
		scf_vector_free(vec);
		return ret;
	}

	if (0 == vec->size) {
		*pv = NULL;
		scf_vector_free(vec);
		return 0;
	}

	scf_variable_t* v2 = vec->data[0];
	scf_variable_t* v;
	int n = 0;
	int i;
	for (i = 0; i < vec->size; i++) {
		v  =        vec->data[i];

		if (!v->extern_flag) {
			v2 = v;
			n++;
		}
	}

	if (n > 1) {
		for (i = 0; i < vec->size; i++) {
			v  =        vec->data[i];

			if (!v->extern_flag)
				scf_loge("multi-define: '%s' in file: %s, line: %d\n", name, v->w->file->data, v->w->line);
		}

		scf_vector_free(vec);
		return -1;
	}

	*pv = v2;

	scf_vector_free(vec);
	return 0;
}

static int _type_check(scf_type_t** pt, scf_vector_t* vec)
{
	scf_type_t* t2 = vec->data[0];
	scf_type_t* t;

	int n = 0;
	int i;

	for (i = 0; i < vec->size; i++) {
		t  =        vec->data[i];

		if (t->node.define_flag) {
			t2 = t;
			n++;
		}
	}

	if (n > 1) {
		for (i = 0; i < vec->size; i++) {
			t  =        vec->data[i];

			if (t->node.define_flag)
				scf_loge("multi-define: '%s' in file: %s, line: %d\n", t->name->data, t->node.w->file->data, t->node.w->line);
		}

		return -1;
	}

	*pt = t2;
	return 0;
}

int scf_ast_find_global_type(scf_type_t** pt, scf_ast_t* ast, char* name)
{
	scf_vector_t* vec = scf_vector_alloc();
	if (!vec)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)ast->root_block, name, vec, -1, _find_type_by_name);
	if (ret < 0) {
		scf_vector_free(vec);
		return ret;
	}

	if (0 == vec->size) {
		*pt = NULL;
		ret = 0;
	} else
		ret = _type_check(pt, vec);

	scf_vector_free(vec);
	return ret;
}

int scf_ast_find_global_type_type(scf_type_t** pt, scf_ast_t* ast, int type)
{
	scf_vector_t* vec = scf_vector_alloc();
	if (!vec)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)ast->root_block, (void*)(intptr_t)type, vec, -1, _find_type_by_type);
	if (ret < 0) {
		scf_vector_free(vec);
		return ret;
	}

	if (0 == vec->size) {
		*pt = NULL;
		ret = 0;
	} else
		ret = _type_check(pt, vec);

	scf_vector_free(vec);
	return ret;
}

int scf_ast_find_function(scf_function_t** pf, scf_ast_t* ast, char* name)
{
	*pf = scf_block_find_function(ast->current_block, name);
	if (*pf)
		return 0;

	return scf_ast_find_global_function(pf, ast, name);
}

int scf_ast_find_variable(scf_variable_t** pv, scf_ast_t* ast, char* name)
{
	*pv = scf_block_find_variable(ast->current_block, name);
	if (*pv)
		return 0;

	return scf_ast_find_global_variable(pv, ast, name);
}

int scf_ast_find_type(scf_type_t** pt, scf_ast_t* ast, char* name)
{
	*pt = scf_block_find_type(ast->current_block, name);
	if (*pt)
		return 0;

	return scf_ast_find_global_type(pt, ast, name);
}

int scf_ast_find_type_type(scf_type_t** pt, scf_ast_t* ast, int type)
{
	*pt = scf_block_find_type_type(ast->current_block, type);
	if (*pt)
		return 0;

	return scf_ast_find_global_type_type(pt, ast, type);
}

