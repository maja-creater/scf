#include"scf_dfa.h"
#include"scf_parse.h"

static int _scf_array_member_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* array,
		intptr_t* indexes, int nb_indexes, scf_node_t** pnode)
{
	if (!pnode) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_operator_t* op_index  = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	scf_type_t*     t_int     = scf_block_find_type_type(ast->current_block, SCF_VAR_INT);
	scf_node_t*     node_root = *pnode;

	if (!node_root) {
		node_root   = scf_node_alloc(NULL, array->type, array);
	}

	printf("%s(), %d, array->nb_dimentions: %d, nb_indexes: %d\n", __func__, __LINE__, array->nb_dimentions, nb_indexes);

	if (nb_indexes < array->nb_dimentions) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;
	}

	int i;
	for (i = 0; i < array->nb_dimentions; i++) {

		int k = indexes[i];

		if (k >= array->dimentions[i]) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_variable_t* v_index     = scf_variable_alloc(w, t_int);
		v_index->const_flag         = 1;
		v_index->const_literal_flag = 1;
		v_index->data.i             = k;

		scf_node_t* node_index      = scf_node_alloc(NULL, v_index->type,  v_index);
		scf_node_t* node_op_index   = scf_node_alloc(w,    op_index->type, NULL);

		scf_node_add_child(node_op_index, node_root);
		scf_node_add_child(node_op_index, node_index);
		node_root = node_op_index;
	}

	*pnode = node_root;
	return array->nb_dimentions;
}

int scf_struct_member_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* _struct,
		intptr_t* indexes, int nb_indexes, scf_node_t** pnode)
{
	if (!pnode) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_type_t*     t         = scf_block_find_type_type(ast->current_block, _struct->type);
	scf_variable_t* v         = NULL;

	scf_operator_t* op_index  = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	scf_type_t*     t_int     = scf_block_find_type_type(ast->current_block, SCF_VAR_INT);
	scf_node_t*     node_root = *pnode;

	if (!node_root) {
		node_root = scf_node_alloc(NULL, _struct->type,  _struct);
	}

	int j = 0;
	while (j < nb_indexes) {

		int k = indexes[j];

		if (!t->scope) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		if (k >= t->scope->vars->size) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		v = t->scope->vars->data[k];

		scf_node_t* node_op_index = scf_node_alloc(w,    op_index->type, NULL);
		scf_node_t* node_v        = scf_node_alloc(NULL, v->type,        v);

		scf_node_add_child(node_op_index, node_root);
		scf_node_add_child(node_op_index, node_v);
		node_root = node_op_index;

		printf("%s(), %d, j: %d, k: %d, v: '%s'\n", __func__, __LINE__, j, k, v->w->text->data);
		j++;

		if (v->nb_dimentions > 0) {

			int ret = _scf_array_member_init(ast, w, v, indexes + j, nb_indexes - j, &node_root);
			if (ret < 0) {
				printf("%s(), %d, error: \n", __func__, __LINE__);
				return -1;
			}

			j += ret;
			printf("%s(), %d, struct var member: %s->%s[]\n", __func__, __LINE__, _struct->w->text->data, v->w->text->data);
		}

		if (v->type < SCF_STRUCT || v->nb_pointers > 0) {
			// if v is a base type var or a pointer, and of course v isn't an array,
			// we can't get the member of v !!
			// the index must be the last one, and its expr is to init v !
			if (j < nb_indexes - 1) {
				printf("%s(), %d, error: \n", __func__, __LINE__);
				return -1;
			}

			printf("%s(), %d, struct var member: %s->%s\n", __func__, __LINE__, _struct->w->text->data, v->w->text->data);

			*pnode = node_root;
			return nb_indexes;
		}

		// v is not a base type var or a pointer, it's a struct
		// first, find the type in this struct scope, then find in global
		scf_type_t* type_v = NULL;
		while (t) {
			type_v = scf_scope_find_type_type(t->scope, v->type);
			if (type_v)
				break;

			// only can use types in this scope, or parent scope
			// can't use types in children scope
			t = t->parent;
		}

		if (!type_v) {
			type_v = scf_block_find_type_type(ast->current_block, v->type);
			if (!type_v) {
				printf("%s(), %d, error: \n", __func__, __LINE__);
				return -1;
			}
		}

		t = type_v;
	}

	// if goto here, the index->size if less than needed, error
	printf("%s(), %d, error: struct var member: %s->%s\n", __func__, __LINE__, _struct->w->text->data, v->w->text->data);
	return -1;
}

int scf_array_member_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* array, intptr_t* indexes, int nb_indexes, scf_node_t** pnode)
{
	scf_type_t*     t = scf_block_find_type_type(ast->current_block, array->type);
	scf_variable_t* v = NULL;

	scf_operator_t* op_index  = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	scf_type_t*     t_int     = scf_block_find_type_type(ast->current_block, SCF_VAR_INT);

	scf_node_t*     node_root = NULL;

	int ret = _scf_array_member_init(ast, w, array, indexes, nb_indexes, &node_root);
	if (ret < 0) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (t->type < SCF_STRUCT || t->nb_pointers > 0) {
		if (ret < nb_indexes - 1) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		*pnode = node_root;
		return nb_indexes;
	}

	ret = scf_struct_member_init(ast, w, array, indexes + ret, nb_indexes - ret, &node_root);
	if (ret < 0) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;
	}

	*pnode = node_root;
	return nb_indexes;
}

int scf_array_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* var, scf_vector_t* init_exprs)
{
	int nb_unset_dims     = 0;
	int unset_dims[8];
	int i;

	for (i = 0; i < var->nb_dimentions; i++) {
		assert(var->dimentions);

		printf("%s(), %d, dim[%d]: %d\n", __func__, __LINE__, i, var->dimentions[i]);

		if (var->dimentions[i] < 0)
			unset_dims[nb_unset_dims++] = i;
	}

	if (nb_unset_dims > 1) {

		printf("%s(), %d, error: \n", __func__, __LINE__);
		return -1;

	} else if (1 == nb_unset_dims) {

		int unset_dim       = unset_dims[0];
		int unset_index_max = -1;

		for (i = 0; i < init_exprs->size; i++) {

			dfa_init_expr_t* init_expr = init_exprs->data[i];

			if ((intptr_t)init_expr->current_index->data[unset_dim] > unset_index_max)
				unset_index_max = (intptr_t)init_expr->current_index->data[unset_dim];

		}

		var->dimentions[unset_dim] = unset_index_max + 1;
	}

	for (i = 0; i < init_exprs->size; i++) {

		dfa_init_expr_t* init_expr = init_exprs->data[i];

		int j;
		for (j = 0; j < var->nb_dimentions; j++) {

			printf("\033[32m%s(), %d, i: %d, dim: %d, size: %d, index: %d\033[0m\n",
					__func__, __LINE__, i, j, var->dimentions[j],
					(int)(intptr_t)(init_expr->current_index->data[j]));

			if ((intptr_t)init_expr->current_index->data[j] >= var->dimentions[j]) {

				printf("%s(), %d, error: index [%d] out of size [%d], in dim: %d\n",
						__func__, __LINE__,
						(int)(intptr_t)init_expr->current_index->data[j], var->dimentions[j], j);

				return -1;
			}
		}
	}

	scf_type_t*     t         = scf_block_find_type_type(ast->current_block,SCF_VAR_INT);
	scf_operator_t* op_index  = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	scf_operator_t* op_assign = scf_find_base_operator_by_type(SCF_OP_ASSIGN);

	for (i = 0; i < init_exprs->size; i++) {

		dfa_init_expr_t* init_expr = init_exprs->data[i];
		printf("%s(), %d, #### data init, i: %d, init_expr->expr: %p\n", __func__, __LINE__, i, init_expr->expr);

		scf_node_t* node_assign = scf_node_alloc(w,    op_assign->type, NULL);
		scf_node_t* node        = NULL;
		intptr_t*   indexes     = (intptr_t*)init_expr->current_index->data;
		int         nb_indexes  = init_expr->current_index->size;

		if (scf_array_member_init(ast, w, var, indexes, nb_indexes, &node) < 0) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_node_add_child(node_assign, node);
		scf_node_add_child(node_assign, init_expr->expr);

		scf_node_add_child((scf_node_t*)ast->current_block, node_assign);

		scf_vector_free(init_expr->current_index);
		free(init_expr);
		init_expr           = NULL;
		init_exprs->data[i] = NULL;
		printf("\n");
	}

	return 0;
}


int scf_struct_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* var, scf_vector_t* init_exprs)
{
	scf_type_t*     t         = scf_block_find_type_type(ast->current_block,SCF_VAR_INT);
	scf_operator_t* op_index  = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	scf_operator_t* op_assign = scf_find_base_operator_by_type(SCF_OP_ASSIGN);

	int i;
	for (i = 0; i < init_exprs->size; i++) {

		dfa_init_expr_t* init_expr = init_exprs->data[i];
		printf("%s(), %d, #### struct init, i: %d, init_expr->expr: %p\n", __func__, __LINE__, i, init_expr->expr);

		scf_node_t* node        = NULL;
		intptr_t*   indexes     = (intptr_t*)init_expr->current_index->data;
		int         nb_indexes  = init_expr->current_index->size;

		if (scf_struct_member_init(ast, w, var, indexes, nb_indexes, &node) < 0) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_node_t* node_assign   = scf_node_alloc(w,    op_assign->type, NULL);

		scf_node_add_child(node_assign, node);
		scf_node_add_child(node_assign, init_expr->expr);

		scf_node_add_child((scf_node_t*)ast->current_block, node_assign);

		scf_vector_free(init_expr->current_index);
		free(init_expr);
		init_expr           = NULL;
		init_exprs->data[i] = NULL;
		printf("\n");
	}

	return 0;
}
