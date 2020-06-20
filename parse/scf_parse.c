#include"scf_parse.h"
#include"scf_native_x64.h"
#include"scf_operator_handler_semantic.h"

scf_base_type_t	base_types[] = {
	{SCF_VAR_CHAR,		"char",		1},
	{SCF_VAR_INT, 		"int",		4},
	{SCF_VAR_DOUBLE, 	"double",	8},
	{SCF_VAR_STRING, 	"string",	-1},
};

#define SCF_CHECK_LEX_POP_WORD(w) \
	do {\
		int ret = scf_lex_pop_word(parse->lex, &w);\
		if (ret < 0) {\
			printf("%s(),%d, error: \n", __func__, __LINE__);\
			return -1;\
		}\
	} while (0)

int _parse_struct_define(scf_parse_t* parse, scf_scope_t* current_scope);
int _parse_var_declaration(scf_parse_t* parse, scf_type_t* t, scf_scope_t* current_scope);
int _parse_expr(scf_parse_t* parse, scf_expr_t* expr);
int _parse_operator(scf_parse_t* parse, scf_expr_t* expr, int nb_operands);
int _parse_identity(scf_parse_t* parse, scf_lex_word_t* w0);
int _parse_word(scf_parse_t* parse, scf_lex_word_t* w);

int	scf_parse_open(scf_parse_t** pparse, const char* path)
{
	assert(pparse);
	assert(path);

	scf_parse_t* parse = calloc(1, sizeof(scf_parse_t));
	assert(parse);

	scf_list_init(&parse->word_list_head);
	scf_list_init(&parse->error_list_head);

	scf_list_init(&parse->code_list_head);

	if (scf_lex_open(&parse->lex, path) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (scf_ast_open(&parse->ast) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int i;
	for (i = 0; i < sizeof(base_types) / sizeof(base_types[0]); i++) {
		scf_ast_add_base_type(parse->ast, &base_types[i]);
	}

	scf_ast_add_file_block(parse->ast, path);

	*pparse = parse;
	return 0;
}

int scf_parse_close(scf_parse_t* parse)
{
	assert(parse);

	free(parse);
	parse = NULL;
	return 0;
}

int _parse_operator_parentheses(scf_parse_t* parse, scf_expr_t* expr, int nb_operands)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	assert(SCF_LEX_WORD_LP == w1->type);

	scf_expr_t* e1 = scf_expr_alloc();
	if (_parse_expr(parse, e1) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	SCF_CHECK_LEX_POP_WORD(w2);
	if (SCF_LEX_WORD_RP != w2->type) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_operator_t* op = scf_find_base_operator("()", 1);
	if (!op) {
		printf("%s(),%d, error: operator not support, w1: %s, line: %d, pos: %d\n",
				__func__, __LINE__,
				w1->text->data, w1->line, w1->pos);
		return -1;
	}
	scf_lex_word_free(w1);
	w1 = NULL;

	e1->type = op->type;
	e1->op = op;
	e1->w = w2;
	e1->nb_nodes = 1;
	w2 = NULL;

	if (scf_expr_add_node(expr, e1) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d, () ok\n", __func__, __LINE__);
	return _parse_operator(parse, expr, 2);
}

int _parse_operator_array_index(scf_parse_t* parse, scf_expr_t* expr, int nb_operands)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	assert(SCF_LEX_WORD_LS == w1->type);
	printf("%s(),%d\n", __func__, __LINE__);

	scf_expr_t* e1 = scf_expr_alloc();
	if (_parse_expr(parse, e1) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d\n", __func__, __LINE__);

	SCF_CHECK_LEX_POP_WORD(w2);
	if (SCF_LEX_WORD_RS != w2->type) {
		printf("%s(),%d, error: w: %s\n", __func__, __LINE__, w2->text->data);
		return -1;
	}
	printf("%s(),%d\n", __func__, __LINE__);

	scf_operator_t* op = scf_find_base_operator("[]", 2);
	if (!op) {
		printf("%s(),%d, error: operator not support, w1: %s, line: %d, pos: %d\n",
				__func__, __LINE__,
				w1->text->data, w1->line, w1->pos);
		return -1;
	}
	scf_lex_word_free(w1);
	w1 = NULL;
	printf("%s(),%d\n", __func__, __LINE__);

	scf_node_t* n0 = scf_node_alloc(w2, op->type, NULL);
	w2 = NULL;
	n0->op = op;
	if (scf_expr_add_node(expr, n0) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* n1 = e1->nodes[0];
	e1->nodes[0] = NULL;
	scf_expr_free(e1);
	e1 = NULL;

	scf_node_add_child(n0, n1);

	printf("%s(),%d, [] ok\n", __func__, __LINE__);
	return _parse_operator(parse, expr, 2);
}

int _parse_operator(scf_parse_t* parse, scf_expr_t* expr, int nb_operands)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);

	if (!scf_lex_is_operator(w1)) {
		printf("%s(),%d, w1: %s\n", __func__, __LINE__, w1->text->data);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	}

	if (SCF_LEX_WORD_LP == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return _parse_operator_parentheses(parse, expr, 1);

	} else if (SCF_LEX_WORD_RP == w1->type) {
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;

	} else if (SCF_LEX_WORD_LS == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return _parse_operator_array_index(parse, expr, 2);

	} else if (SCF_LEX_WORD_RS == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_LB == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_RB == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	}

	scf_operator_t* op = scf_find_base_operator(w1->text->data, nb_operands);
	if (!op) {
		printf("%s(),%d, error: operator not support, w1: %s, line: %d, pos: %d\n",
				__func__, __LINE__,
				w1->text->data, w1->line, w1->pos);
		return -1;
	}
	printf("%s(),%d, w1: %s, type: %d\n", __func__, __LINE__, w1->text->data, op->type);

	scf_node_t* node = scf_node_alloc(w1, op->type, NULL);
	node->op = op;
	w1 = NULL;

	if (scf_expr_add_node(expr, node) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return _parse_expr(parse, expr);
}

int _parse_function_call_args(scf_parse_t* parse, scf_node_t* call)
{
	printf("%s(),%d\n", __func__, __LINE__);
	scf_expr_t* e = scf_expr_alloc();
	if (_parse_expr(parse, e) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("%s(),%d\n", __func__, __LINE__);

	scf_node_t* n = e->nodes[0];
	e->nodes[0] = NULL;
	scf_expr_free(e);
	e = NULL;

	if (n) {
		scf_node_add_child(call, n);
	}

	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);

	if (SCF_LEX_WORD_RP == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w1);
		w1 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_COMMA == w1->type) {
		scf_lex_word_free(w1);
		w1 = NULL;
		return _parse_function_call_args(parse, call);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_function_call(scf_parse_t* parse, scf_expr_t* expr, scf_lex_word_t* w0)
{
	scf_function_t* f = scf_ast_find_function(parse->ast, w0->text->data);
	if (!f) {
		printf("%s(),%d, error: %s()\n", __func__, __LINE__, w0->text->data);
		printf("%s(),%d, ### call: current_block: %p\n", __func__, __LINE__, parse->ast->current_block);
		return -1;
	}

	scf_node_t* call = scf_node_alloc(w0, SCF_OP_CALL, NULL);

	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);
	assert(SCF_LEX_WORD_LP == w1->type);

	if (_parse_function_call_args(parse, call) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (call->nb_nodes != f->argv->size) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_label_t* l = scf_label_alloc(NULL);
	l->node = (scf_node_t*)f;

	scf_node_t* n = scf_node_alloc_label(l);
	scf_node_add_child(call, n);

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_CALL);
	call->op = op;
	scf_expr_add_node(expr, call);

	return _parse_operator(parse, expr, 2);
}

int _parse_expr(scf_parse_t* parse, scf_expr_t* expr)
{
	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);
	//printf("%s(),%d, w1->text->data: %s\n", __func__, __LINE__, w1->text->data);

	if (SCF_LEX_WORD_NUMBER_CHAR == w1->type
			|| SCF_LEX_WORD_NUMBER_INT == w1->type) {

		scf_type_t* t = NULL;

		if (SCF_LEX_WORD_NUMBER_CHAR == w1->type) {
			// const char 
			t	= scf_ast_find_type(parse->ast, "char");

		} else if (SCF_LEX_WORD_NUMBER_INT == w1->type) {
			// const int
			t = scf_ast_find_type(parse->ast, "int");
		}

		scf_variable_t*	var = scf_variable_alloc(w1, t);
		var->const_flag 		= 1;
		var->const_literal_flag	= 1;
		var->data.i				= w1->data.i;

		printf("%s(),%d, w1->text->data: %s\n", __func__, __LINE__, w1->text->data);
		w1 = NULL;

		scf_node_t* node = scf_node_alloc(NULL, var->type, var);

		if (scf_expr_add_node(expr, node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		return _parse_operator(parse, expr, 2); // parse binary operator

	} else if (SCF_LEX_WORD_NUMBER_DOUBLE == w1->type) {
		// const double 
		printf("%s(),%d\n", __func__, __LINE__);
		scf_type_t*			t 	= scf_ast_find_type(parse->ast, "double");
		scf_variable_t*		var = scf_variable_alloc(w1, t);
		var->const_literal_flag	= 1;
		var->const_flag 		= 1;
		var->data.d				= w1->data.d;

		scf_lex_word_free(w1);
		w1 = NULL;

		scf_node_t* node = scf_node_alloc(NULL, var->type, var);

		if (scf_expr_add_node(expr, node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		return _parse_operator(parse, expr, 2); // parse binary operator

	} else if (SCF_LEX_WORD_STRING == w1->type) {
		// const string
		printf("%s(),%d\n", __func__, __LINE__);
		scf_type_t*			t 	= scf_ast_find_type(parse->ast, "char");

		int nb_pointers 		= t->nb_pointers;
		t->nb_pointers			= 1;
		scf_variable_t*		var = scf_variable_alloc(w1, t);
		t->nb_pointers			= nb_pointers;

		var->const_literal_flag	= 1;
		var->const_flag 		= 1;
		var->data.s				= scf_string_clone(w1->data.s);

		printf("%s(),%d, w1: %p, var->data.s: %s\n", __func__, __LINE__, w1, var->data.s->data);
//		scf_lex_word_free(w1);
		w1 = NULL;

		printf("%s(),%d, var->data.s: %s\n", __func__, __LINE__, var->data.s->data);
		scf_node_t* node = scf_node_alloc(NULL, var->type, var);

		if (scf_expr_add_node(expr, node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		return _parse_operator(parse, expr, 2); // parse binary operator

	} else if (scf_lex_is_identity(w1)) {
		printf("%s(),%d, w1->text->data: %s\n", __func__, __LINE__, w1->text->data);

		scf_lex_word_t* w2 = NULL;
		SCF_CHECK_LEX_POP_WORD(w2);
		if (SCF_LEX_WORD_LP == w2->type) {
			// function call
			scf_lex_push_word(parse->lex, w2);
			w2 = NULL;
			return _parse_function_call(parse, expr, w1);
		} else {
			scf_lex_push_word(parse->lex, w2);
			w2 = NULL;
		}

		// variable, should be declared before
		scf_variable_t* var = scf_ast_find_variable(parse->ast, w1->text->data);
		if (!var) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		// w1 is not used now
		scf_lex_word_free(w1);
		w1 = NULL;

		// alloc a new node to add to the expr
		scf_node_t* node = scf_node_alloc(NULL, var->type, var);
		if (scf_expr_add_node(expr, node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return _parse_operator(parse, expr, 2); // parse binary operator

	} else if (SCF_LEX_WORD_SEMICOLON == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w1);
		w1 = NULL;
		return 0;

	} else if (SCF_LEX_WORD_RS == w1->type) {
		// ']'
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_RP == w1->type) {
		// ')'
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_COMMA == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		return 0;
	}

	// from here, should be unary operator
	printf("%s(),%d, w1->text->data: %s\n", __func__, __LINE__, w1->text->data);
	scf_lex_push_word(parse->lex, w1);
	w1 = NULL;

	int ret = _parse_operator(parse, expr, 1);
	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
	}
	return ret;
}

int _parse_var_array_init(scf_parse_t* parse, scf_variable_t* var, scf_type_t* t, scf_scope_t* current_scope)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	if (SCF_LEX_WORD_LB != w1->type) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_vector_t* vec = scf_vector_alloc();

	while (1) {
		SCF_CHECK_LEX_POP_WORD(w2);

		if (SCF_LEX_WORD_RB == w2->type) {
			printf("%s(),%d\n", __func__, __LINE__);
			break;
		}

		if (SCF_LEX_WORD_COMMA == w2->type) {
			scf_lex_word_free(w2);
			w2 = NULL;
			continue;
		} else if (SCF_LEX_WORD_SEMICOLON == w2->type) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;

		scf_expr_t* expr = scf_expr_alloc();
		if (_parse_expr(parse, expr) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_variable_t* result = NULL;
		scf_expr_calculate(parse->ast, expr, &result);
		scf_variable_print(result);
		scf_vector_add(vec, result);
		result = NULL;

		scf_expr_free(expr);
		expr = NULL;
	}

	int i;
	int n = 0;
	int j = -1;
	int k = 1;
	for (i = 0; i < var->nb_dimentions; i++) {
		if (var->dimentions[i] < 0) {
			n++;
			j = i;
		} else if (0 == var->dimentions[i]) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		} else {
			k *= var->dimentions[i];
		}
	}
	if (n > 1) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	if (1 == n) {
		assert(j >= 0);
		if (vec->size % k != 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		var->dimentions[j] = vec->size / k;
	}

	scf_variable_alloc_space(var);

	for (i = 0; i < vec->size; i++) {
		printf("%s(),%d, i: %d\n", __func__, __LINE__, i);
		scf_variable_set_array_member(var, i, vec->data[i]);
	}

	return 0;
}

int _parse_var_array(scf_parse_t* parse, scf_variable_t* var, scf_type_t* t, scf_scope_t* current_scope)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	assert(SCF_LEX_WORD_LS == w1->type);

	SCF_CHECK_LEX_POP_WORD(w2);

	if (SCF_LEX_WORD_RS == w2->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w2);
		w2 = NULL;

		// set this dimention -1, then determine it when init array
		scf_variable_add_array_dimention(var, -1);
	} else {
		// it should be an expr for the array dimention size
		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;

		scf_expr_t* expr = scf_expr_alloc();

		printf("%s(),%d\n", __func__, __LINE__);
		int ret = _parse_expr(parse, expr);
		printf("%s(),%d\n", __func__, __LINE__);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_lex_word_t* w4 = NULL;
		SCF_CHECK_LEX_POP_WORD(w4);
		printf("%s(),%d, w4: %s\n", __func__, __LINE__, w4->text->data);

		assert(0 == ret);
		printf("%s(),%d\n", __func__, __LINE__);

		scf_variable_t* result = NULL;
		scf_expr_calculate(parse->ast, expr, &result);
		assert(result);
		assert(SCF_VAR_INT == result->type);

		scf_variable_add_array_dimention(var, result->data.i);

		printf("%s(),%d, result: %p\n", __func__, __LINE__, result);
		printf("%s(),%d, var: %p\n", __func__, __LINE__, var);
		scf_variable_print(result);
		printf("%s(),%d\n", __func__, __LINE__);

		scf_variable_free(result);
		result = NULL;

		printf("%s(),%d\n", __func__, __LINE__);
		scf_variable_print(var);
		printf("%s(),%d\n", __func__, __LINE__);
	}

	scf_lex_word_t* w3 = NULL;
	SCF_CHECK_LEX_POP_WORD(w3);
	if (SCF_LEX_WORD_COMMA == w3->type) {
		scf_lex_word_free(w3);
		w3 = NULL;
		return _parse_var_declaration(parse, t, current_scope);

	} else if (SCF_LEX_WORD_ASSIGN == w3->type) {
		// array init
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w3);
		w3 = NULL;

		return _parse_var_array_init(parse, var, t, current_scope);

	} else if (SCF_LEX_WORD_SEMICOLON == w3->type) {
		scf_lex_word_free(w3);
		w3 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_LS == w3->type) {
		// multi-dimention array
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w3);
		w3 = NULL;
		return _parse_var_array(parse, var, t, current_scope);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_var_declaration(scf_parse_t* parse, scf_type_t* t, scf_scope_t* current_scope)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);

	if (SCF_LEX_WORD_STAR == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		// pointer var of type t, code like: t *
		scf_lex_word_free(w1);
		w1 = NULL;

		t->nb_pointers++;
		int ret = _parse_var_declaration(parse, t, current_scope);
		t->nb_pointers--;

		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
		}
		return ret;

	} else if (SCF_LEX_WORD_SEMICOLON == w1->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w1);
		w1 = NULL;
		return 0;
	}

	if (!scf_lex_is_identity(w1)) {
		printf("%s(),%d, error: w1->text->data: %s\n", __func__, __LINE__, w1->text->data);
		return -1;
	}

	scf_variable_t*	var = scf_scope_find_variable(current_scope, w1->text->data);
	if (var) {
		printf("%s(),%d, error: var '%s' already exist, line: %d\n", __func__, __LINE__, w1->text->data, var->w->line);
		return -1;
	}

	var = scf_variable_alloc(w1, t);
	w1 = NULL;
	scf_scope_push_var(current_scope, var);
#if 1
	printf("%s(),%d, var: name: %s, type name: %s, type: %d, size: %d, offset: %d, nb_pointers: %d\n",
			__func__, __LINE__,
			var->w->text->data, t->name->data, var->type, var->size, var->offset, var->nb_pointers
			);
#endif
	SCF_CHECK_LEX_POP_WORD(w2);
	if (SCF_LEX_WORD_COMMA == w2->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w2);
		w2 = NULL;

		int nb_pointers = t->nb_pointers;
		t->nb_pointers = 0;
		int ret = _parse_var_declaration(parse, t, current_scope);
		t->nb_pointers = nb_pointers;
		return ret;

	} else if (SCF_LEX_WORD_ASSIGN == w2->type) {
		scf_expr_t* expr = scf_expr_alloc();

		scf_node_t* var_node = scf_node_alloc(NULL, var->type, var);
		scf_node_t* assign_node = scf_node_alloc(w2, SCF_OP_ASSIGN, NULL);
		w2 = NULL;

		if (scf_expr_add_node(expr, var_node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		if (scf_expr_add_node(expr, assign_node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		printf("%s(),%d\n", __func__, __LINE__);
		int ret = _parse_expr(parse, expr);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
		} else if (0 == ret) {
			scf_block_t* b = parse->ast->current_block;
			scf_node_add_child((scf_node_t*)b, expr);
			printf("%s(),%d, b: %p, expr: %p\n", __func__, __LINE__, b, expr);
#if 0
			scf_variable_t* result = NULL;

			scf_expr_calculate(parse, expr, &result);
			assert(result);
			scf_variable_print(result);
			scf_variable_free(result);
			result = NULL;

			printf("%s(),%d\n", __func__, __LINE__);
			scf_variable_print(var);
#endif
		}
		return ret;

	} else if (SCF_LEX_WORD_SEMICOLON == w2->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_word_free(w2);
		w2 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_LS == w2->type) {
		printf("%s(),%d\n", __func__, __LINE__);
		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;
		return _parse_var_array(parse, var, t, current_scope);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_struct_members(scf_parse_t* parse, scf_type_t* t, scf_scope_t* current_scope)
{
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	if (SCF_LEX_WORD_KEY_STRUCT == w1->type) {
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		int ret = _parse_struct_define(parse, current_scope);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
		}
		return _parse_struct_members(parse, t, current_scope);

	} else if (SCF_LEX_WORD_RB == w1->type) {
		// current struct definition ended 
		scf_lex_word_free(w1);
		w1 = NULL;

		int size = 0;
		scf_list_t* l;
		for (l = scf_list_tail(&current_scope->var_list_head);
				l != scf_list_sentinel(&current_scope->var_list_head);
				l = scf_list_prev(l)) {

			scf_variable_t* v = scf_list_data(l, scf_variable_t, list);

			assert(v->size > 0);
			v->offset = (size + v->size - 1) / v->size * v->size;
			size = v->offset + v->size;
#if 1
			printf("%s(),%d, var: type: %d, name: %s, size: %d, offset: %d, nb_pointers: %d\n",
					__func__, __LINE__,
					v->type, v->w->text->data, v->size, v->offset, v->nb_pointers);
#endif
		}
		t->size = size;
#if 1
		printf("%s(),%d, t: %d, name: %s, size: %d\n", __func__, __LINE__,
				t->type, t->name->data, t->size);
#endif
		return 0;
	}

	scf_type_t*	t1 = scf_scope_find_type(t->scope, w1->text->data);
	if (!t1) {
		t1 = scf_ast_find_type(parse->ast, w1->text->data);
		if (!t1) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}
	scf_lex_word_free(w1);
	w1 = NULL;

	if (_parse_var_declaration(parse, t1, current_scope) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return _parse_struct_members(parse, t, current_scope);
}

int _parse_struct_define(scf_parse_t* parse, scf_scope_t* current_scope)
{
	scf_lex_word_t* w0 = NULL;
	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;

	SCF_CHECK_LEX_POP_WORD(w0);
	assert(SCF_LEX_WORD_KEY_STRUCT == w0->type);
	scf_lex_word_free(w0);
	w0 = NULL;

	SCF_CHECK_LEX_POP_WORD(w1);
	if (!scf_lex_is_identity(w1)) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_type_t*		t		= scf_scope_find_type(current_scope, w1->text->data);

	SCF_CHECK_LEX_POP_WORD(w2);
	if (SCF_LEX_WORD_SEMICOLON == w2->type) {
		// only declare a new struct

		if (!t) {
			// if not exist this type in current scope, add it
			t = scf_type_alloc(w1, w1->text->data, SCF_STRUCT + parse->ast->nb_structs, 0);
			w1 = NULL;
			scf_scope_push_type(current_scope, t);
			parse->ast->nb_structs++;
		} else {
			scf_lex_word_free(w1);
			w1 = NULL;
		}

		scf_lex_word_free(w2);
		w2 = NULL;
		return 0;
	} else if (SCF_LEX_WORD_STAR == w2->type) {
		// define a struct pointer
		if (!t) {
			t = scf_ast_find_type(parse->ast, w1->text->data);
			if (!t) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}
		}

		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;

		scf_lex_word_free(w1);
		w1 = NULL;
		return _parse_var_declaration(parse, t, current_scope);

	} else if (SCF_LEX_WORD_LB == w2->type) {
		// define a struct type

		if (!t) {
			// if not exist this type in current scope, add it
			t = scf_type_alloc(w1, w1->text->data, SCF_STRUCT + parse->ast->nb_structs, 0);
			w1 = NULL;
			scf_scope_push_type(current_scope, t);
			parse->ast->nb_structs++;
		} else {
			scf_lex_word_free(w1);
			w1 = NULL;
		}

		scf_scope_t* struct_scope = scf_scope_alloc(w2, t->name->data);
		w2 = NULL;

		//add struct scope to type's scope_list
		t->scope = struct_scope;
		if (_parse_struct_members(parse, t, struct_scope) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		return _parse_var_declaration(parse, t, current_scope);

	} else if (scf_lex_is_identity(w2)) {
		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;
		return _parse_var_declaration(parse, t, current_scope);

	} else if (SCF_LEX_WORD_RB == w2->type) {
		scf_lex_word_free(w2);
		w2 = NULL;
		return _parse_var_declaration(parse, t, current_scope);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_block(scf_parse_t* parse)
{
	scf_lex_word_t* w0 = NULL;
	SCF_CHECK_LEX_POP_WORD(w0);
	assert(SCF_LEX_WORD_LB == w0->type);

	scf_block_t* b = scf_block_alloc(w0);
	scf_ast_push_block(parse->ast, b);
	printf("%s(),%d, b: %p, b->node.parent: %p\n", __func__, __LINE__, b, b->node.parent);

	while (1) {
		scf_lex_word_t* w1 = NULL;
		SCF_CHECK_LEX_POP_WORD(w1);

		if (SCF_LEX_WORD_RB == w1->type) {
			b->w_end = w1;
			w1 = NULL;
			break;
		}

		if (_parse_word(parse, w1) < 0) {
			printf("%s(),%d, error: w: %d, '%s', w->line: %d, w->pos: %d\n", __func__, __LINE__, w1->type, w1->text->data, w1->line, w1->pos);
			return -1;
		}
	}

	scf_ast_pop_block(parse->ast);
	printf("%s(),%d\n", __func__, __LINE__);
	return 0;
}

int _parse_while_body(scf_parse_t* parse)
{
	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);
	if (SCF_LEX_WORD_LB == w1->type) {
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;

		if (_parse_block(parse) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	} else {
		// parse single statement, can't define variable here
		if ((SCF_LEX_WORD_KEY_CHAR <= w1->type && SCF_LEX_WORD_KEY_DOUBLE >= w1->type)
				|| SCF_LEX_WORD_KEY_STRUCT == w1->type) {

			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		} else if (scf_lex_is_identity(w1)) {
			// identity can't be a defined type
			scf_type_t* t = scf_ast_find_type(parse->ast, w1->text->data);
			if (t) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}

			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
}

int _parse_while(scf_parse_t* parse, scf_node_t** pnode)
{
	scf_lex_word_t* w0 = NULL;
	SCF_CHECK_LEX_POP_WORD(w0);
	printf("\n%s(),%d, ## start: %s, line: %d\n", __func__, __LINE__, w0->text->data, w0->line);
	assert(SCF_LEX_WORD_KEY_WHILE == w0->type);

	scf_expr_t* e = scf_expr_alloc();
	if (_parse_expr(parse, e) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* n = scf_node_alloc(w0, SCF_OP_WHILE, NULL);
	printf("%s(),%d ## n: %p, %s, e: %p\n", __func__, __LINE__, n, n->w->text->data, e);
	scf_node_add_child(n, e);
	scf_ast_push_block(parse->ast, (scf_block_t*)n);

	if (_parse_while_body(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_ast_pop_block(parse->ast);
	w0 = NULL;
	*pnode = n;
	printf("%s(),%d ## n: %p, %s\n", __func__, __LINE__, n, n->w->text->data);
	return 0;
}

int _parse_if_body(scf_parse_t* parse)
{
	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);
	if (SCF_LEX_WORD_LB == w1->type) {
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;

		if (_parse_block(parse) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	} else {
		// parse single statement, can't define variable here
		if ((SCF_LEX_WORD_KEY_CHAR <= w1->type && SCF_LEX_WORD_KEY_DOUBLE >= w1->type)
				|| SCF_LEX_WORD_KEY_STRUCT == w1->type) {

			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		} else if (scf_lex_is_identity(w1)) {
			// identity can't be a defined type
			scf_type_t* t = scf_ast_find_type(parse->ast, w1->text->data);
			if (t) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}

			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
}

int _parse_if(scf_parse_t* parse, scf_node_t** pnode)
{
	scf_lex_word_t* w0 = NULL;
	SCF_CHECK_LEX_POP_WORD(w0);
	printf("\n%s(),%d, ## start: %s, line: %d\n", __func__, __LINE__, w0->text->data, w0->line);
	assert(SCF_LEX_WORD_KEY_IF == w0->type);

	scf_expr_t* e = scf_expr_alloc();
	if (_parse_expr(parse, e) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* n = scf_node_alloc(w0, SCF_OP_IF, NULL);
	printf("%s(),%d ## n: %p, %s, e: %p\n", __func__, __LINE__, n, n->w->text->data, e);
	scf_node_add_child(n, e);
	scf_ast_push_block(parse->ast, (scf_block_t*)n);

	if (_parse_if_body(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	//parse 'else' statement when exist
	scf_lex_word_t* w2 = NULL;
	SCF_CHECK_LEX_POP_WORD(w2);
	if (SCF_LEX_WORD_KEY_ELSE != w2->type) {
		// end to parse 'if'
		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;

		scf_ast_pop_block(parse->ast); // return to the parent block of 'if'
		w0 = NULL;
		*pnode = n;
		printf("%s(),%d ## n: %p, %s\n", __func__, __LINE__, n, n->w->text->data);
		return 0;
	}
	scf_lex_word_free(w2);
	w2 = NULL;

	scf_lex_word_t* w3 = NULL;
	SCF_CHECK_LEX_POP_WORD(w3);

	if (SCF_LEX_WORD_KEY_IF == w3->type) {
		scf_lex_push_word(parse->lex, w3);
		w3 = NULL;

		// recursive parse 'if'
		if (_parse_if(parse, pnode) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		*pnode = n;
		printf("%s(),%d ## n: %p, %s\n", __func__, __LINE__, n, n->w->text->data);
		return 0;
	}

	scf_lex_push_word(parse->lex, w3);
	w3 = NULL;
	if (_parse_if_body(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_ast_pop_block(parse->ast); // return to the parent block of 'if'
	printf("\n%s(),%d, ## end: %s, line: %d\n", __func__, __LINE__, w0->text->data, w0->line);
	w0 = NULL;
	*pnode = n;
	return 0;
}

int _parse_identity(scf_parse_t* parse, scf_lex_word_t* w0)
{
	scf_lex_word_t* w1 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);

	if (scf_lex_is_identity(w1)) {
		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;

		scf_type_t* t = scf_ast_find_type(parse->ast, w0->text->data);
		if (t) {
			printf("%s(),%d\n", __func__, __LINE__);
			scf_lex_word_free(w0);
			w0 = NULL;
			scf_scope_t* current_scope = scf_ast_current_scope(parse->ast);
			return _parse_var_declaration(parse, t, current_scope);
		} else {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	} else if (scf_lex_is_operator(w1)) {

		scf_lex_push_word(parse->lex, w1);
		w1 = NULL;
		scf_lex_push_word(parse->lex, w0);
		w0 = NULL;

		scf_expr_t* e = scf_expr_alloc();
		if (_parse_expr(parse, e) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_block_t* b = parse->ast->current_block;
		scf_node_add_child((scf_node_t*)b, e);
		printf("%s(),%d, b: %p, e: %p\n", __func__, __LINE__, b, e);
#if 0
		scf_variable_t* result = NULL;
		scf_expr_calculate(parse, e, &result);
		scf_variable_print(result);
#endif
		return 0;

	} else if (SCF_LEX_WORD_COLON == w1->type) {
		printf("%s(),%d, parse label: '%s', line: %d, pos: %d\n", __func__, __LINE__, w0->text->data, w0->line, w0->pos);

		scf_lex_word_free(w1);
		w1 = NULL;

		scf_scope_t* current_scope = scf_ast_current_scope(parse->ast);
		scf_label_t* l = scf_scope_find_label(current_scope, w0->text->data);
		if (l) {
			printf("%s(),%d, error: label '%s' already exist in current scope\n",
					__func__, __LINE__, w0->text->data);
			return -1;
		}

		l = scf_label_alloc(w0);
		w0 = NULL;

		scf_node_t* n = scf_node_alloc_label(l);
		l->node = n;

		scf_node_add_child((scf_node_t*)parse->ast->current_block, n);
		scf_scope_push_label(current_scope, l);
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_function_body(scf_parse_t* parse, scf_function_t* f)
{
	scf_lex_word_t* w0 = NULL;

	SCF_CHECK_LEX_POP_WORD(w0);

	if (SCF_LEX_WORD_SEMICOLON == w0->type) {
		if (f->node.nb_nodes > 0) {
			f->w_end = w0;
			w0 = NULL;

			printf("%s(),%d, ### end function: %s, %s\n", __func__, __LINE__,
					f->node.w->text->data,
					f->w_end->text->data
					);
		} else {
			scf_lex_word_free(w0);
			w0 = NULL;
		}
		return 0;
	} else if (SCF_LEX_WORD_LB == w0->type) {
		scf_lex_push_word(parse->lex, w0);
		w0 = NULL;

		printf("%s(),%d, ### start, current_block: %p, f: %p\n", __func__, __LINE__, parse->ast->current_block, f);
		return _parse_block(parse);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_function_args(scf_parse_t* parse, scf_function_t* f)
{
	printf("%s(),%d, ### start, current_block: %p, f: %p\n", __func__, __LINE__, parse->ast->current_block, f);
	scf_type_t* t = NULL;
	scf_lex_word_t* w0 = NULL;
	scf_lex_word_t* w1 = NULL;

	SCF_CHECK_LEX_POP_WORD(w0);

	if ((SCF_LEX_WORD_KEY_CHAR <= w0->type &&
			SCF_LEX_WORD_KEY_DOUBLE >= w0->type)
		|| scf_lex_is_identity(w0)) {

		t = scf_ast_find_type(parse->ast, w0->text->data);

	} else if (SCF_LEX_WORD_KEY_STRUCT == w0->type) {
		SCF_CHECK_LEX_POP_WORD(w1);

		t = scf_ast_find_type(parse->ast, w1->text->data);

	} else if (SCF_LEX_WORD_RP == w0->type) {
		// end arg list
		scf_lex_word_free(w0);
		w0 = NULL;
		if (f->arg_index != f->argv->size) {
			printf("%s(),%d, error: arg numbers not same: %d, %d\n", __func__, __LINE__,
					f->arg_index, f->argv->size);
			return -1;
		}
		return 0;

	} else {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (!t) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	assert(0 == t->nb_pointers);
	scf_vector_t* vec = scf_vector_alloc();
	scf_lex_word_t* w2 = NULL;

	while (1) {
		SCF_CHECK_LEX_POP_WORD(w2);
		if (SCF_LEX_WORD_STAR == w2->type) {
			scf_vector_add(vec, w2);
			w2 = NULL;
		} else {
			break;
		}
	}
	assert(w2);

	scf_variable_t* arg = NULL;

	if (scf_lex_is_identity(w2)) {

		scf_variable_t* v = scf_ast_find_variable(parse->ast, w2->text->data);
		if (v) {
			printf("%s(),%d, error: arg '%s' already exist at line: %d, pos: %d\n", __func__, __LINE__,
					w2->text->data, v->w->line, v->w->pos);
			return -1;
		}

		t->nb_pointers = vec->size;
		arg	= scf_variable_alloc(w2, t);
		w2 = NULL;
		t->nb_pointers = 0;

		SCF_CHECK_LEX_POP_WORD(w2); // read next word, should be ',' or ')'
	} else {
		t->nb_pointers = vec->size;
		arg	= scf_variable_alloc(NULL, t);
		t->nb_pointers = 0;
	}

	if (f->arg_index == f->argv->size) {
		// first parse this arg
		scf_vector_add(f->argv, arg);
		scf_scope_push_var(f->scope, arg);

	} else if (f->arg_index < f->argv->size) {
		// check if same type
		scf_variable_t* v = f->argv->data[f->arg_index];
		if (v->type != arg->type || v->nb_pointers != arg->nb_pointers) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		// update the arg name
		if (v->w) {
			scf_lex_word_free(v->w);
			v->w = NULL;
		}
		v->w = arg->w;
		arg->w = NULL;
	} else {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (SCF_LEX_WORD_COMMA == w2->type) {
		scf_lex_word_free(w2);
		w2 = NULL;
		f->arg_index++;
		return _parse_function_args(parse, f);

	} else if (SCF_LEX_WORD_RP == w2->type) {
		// end the argment list
		scf_lex_word_free(w2);
		w2 = NULL;
		f->arg_index++;
		if (f->arg_index != f->argv->size) {
			printf("%s(),%d, error: arg numbers not same: %d, %d\n", __func__, __LINE__,
					f->arg_index, f->argv->size);
			return -1;
		}
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int _parse_function(scf_parse_t* parse, scf_lex_word_t* w0)
{
	printf("%s(),%d, ### start, current_block: %p\n", __func__, __LINE__, parse->ast->current_block);
	scf_type_t* t = NULL;
	scf_lex_word_t* w1 = NULL;

	if (SCF_LEX_WORD_KEY_CHAR <= w0->type &&
			SCF_LEX_WORD_KEY_DOUBLE >= w0->type) {

		t = scf_ast_find_type(parse->ast, w0->text->data);

	} else if (SCF_LEX_WORD_KEY_STRUCT == w0->type) {
		SCF_CHECK_LEX_POP_WORD(w1);

		t = scf_ast_find_type(parse->ast, w1->text->data);
		if (!t) {
			scf_lex_push_word(parse->lex, w1);
			w1 = NULL;
			return 0;
		}
	} else if (scf_lex_is_identity(w0)) {
		t = scf_ast_find_type(parse->ast, w0->text->data);
		if (!t) {
			scf_variable_t* v = scf_ast_find_variable(parse->ast, w0->text->data);
			if (!v) {
				scf_function_t* f = scf_ast_find_function(parse->ast, w0->text->data);
				if (!f) {
					SCF_CHECK_LEX_POP_WORD(w1);
					if (SCF_LEX_WORD_COLON != w1->type) {
						printf("%s(),%d, error: \n", __func__, __LINE__);
						return -1;
					}

					scf_lex_push_word(parse->lex, w1);
					w1 = NULL;
				}
			}

			return 0;
		}
	} else {
		return 0;
	}

	if (!t) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	assert(0 == t->nb_pointers);
	scf_vector_t* vec = scf_vector_alloc();
	scf_lex_word_t* w2 = NULL;

	while (1) {
		SCF_CHECK_LEX_POP_WORD(w2);
		if (SCF_LEX_WORD_STAR == w2->type) {
			scf_vector_add(vec, w2);
			w2 = NULL;
		} else {
			break;
		}
	}

	assert(w2);

	scf_lex_word_t* w3 = NULL;
	SCF_CHECK_LEX_POP_WORD(w3);

	if (!scf_lex_is_identity(w2) || SCF_LEX_WORD_LP != w3->type) {
		// push word back to lex, for other parse
		scf_lex_push_word(parse->lex, w3);
		w3 = NULL;

		scf_lex_push_word(parse->lex, w2);
		w2 = NULL;

		int i;
		for (i = 0; i < vec->size; i++) {
			scf_lex_word_t* w = vec->data[i];
			scf_lex_push_word(parse->lex, vec->data[i]);
			vec->data[i] = NULL;
		}
		scf_vector_free(vec);
		vec = NULL;

		if (w1) {
			scf_lex_push_word(parse->lex, w1);
			w1 = NULL;
		}
		return 0;
	}

	if (SCF_LEX_WORD_LP != w3->type) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_scope_t* current_scope = scf_ast_current_scope(parse->ast);
	scf_function_t* f = scf_scope_find_function(current_scope, w2->text->data);
	if (!f) {
		printf("%s(),%d, ######## start, current_block: %p, f: %p, %s\n", __func__, __LINE__, parse->ast->current_block, f, w2->text->data);
		f = scf_function_alloc(w2);
		w2 = NULL;

		t->nb_pointers = vec->size;
		f->ret = scf_variable_alloc(w0, t);
		w0 = NULL;
		t->nb_pointers = 0;

		scf_scope_push_function(current_scope, f);
		printf("%s(),%d, ######## start, current_block: %p, f: %p\n", __func__, __LINE__, parse->ast->current_block, f);
		scf_ast_push_block(parse->ast, (scf_block_t*)f);
		printf("%s(),%d, ######## start, current_block: %p, f: %p\n", __func__, __LINE__, parse->ast->current_block, f);
	} else {
		assert(f->scope);
		assert(!f->w_end);

		if (f->ret->type != t->type || f->ret->nb_pointers != t->nb_pointers) {
			printf("%s(),%d, error: return value type not same: %s:%d, %s:%d\n",
					__func__, __LINE__,
					f->ret->w->text->data, f->ret->w->line,
					t->w->text->data, t->w->line
					);
			return -1;
		}

		parse->ast->current_block = (scf_block_t*)f;
		printf("%s(),%d, ### start, current_block: %p, f: %p\n", __func__, __LINE__, parse->ast->current_block, f);
	}

	f->arg_index = 0;
	if (_parse_function_args(parse, f) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (w1) {
		scf_lex_word_free(w1);
		w1 = NULL;
	}

	int i;
	for (i = 0; i < vec->size; i++) {
		scf_lex_word_free(vec->data[i]);
		vec->data[i] = NULL;
	}
	scf_vector_free(vec);
	vec = NULL;

	if (_parse_function_body(parse, f) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// return to the parent block of this function
	scf_ast_pop_block(parse->ast);
	return 1;
}

int _parse_return(scf_parse_t* parse, scf_lex_word_t* w0)
{
	assert(SCF_LEX_WORD_KEY_RETURN == w0->type);

	scf_block_t* b = parse->ast->current_block;
	while (b && SCF_FUNCTION != b->node.type) {
		b = (scf_block_t*)b->node.parent;
	}

	if (!b) {
		printf("%s(),%d, error: 'return' must be in a function, line: %d\n",
				__func__, __LINE__, w0->line);
		return -1;
	}
	assert(SCF_FUNCTION == b->node.type);

	scf_node_t* n = scf_node_alloc(w0, SCF_OP_RETURN, NULL);

	scf_expr_t* e = scf_expr_alloc();
	if (_parse_expr(parse, e) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_add_child(n, e);

	scf_node_add_child((scf_node_t*)parse->ast->current_block, n);
	return 0;
}

int _parse_break(scf_parse_t* parse, scf_lex_word_t* w0)
{
	assert(SCF_LEX_WORD_KEY_BREAK == w0->type);

	scf_block_t* b = parse->ast->current_block;

	scf_node_t* _break = scf_node_alloc(w0, SCF_OP_BREAK, NULL);

	printf("%s(),%d, b: %p, _break: %p\n", __func__, __LINE__, b, _break);

	scf_node_add_child((scf_node_t*)b, _break);
	return 0;
}

int _parse_continue(scf_parse_t* parse, scf_lex_word_t* w0)
{
	assert(SCF_LEX_WORD_KEY_CONTINUE == w0->type);

	scf_node_t* _continue = scf_node_alloc(w0, SCF_OP_CONTINUE, NULL);

	scf_node_add_child((scf_node_t*)parse->ast->current_block, _continue);
	return 0;
}

int _parse_goto(scf_parse_t* parse, scf_lex_word_t* w0)
{
	assert(SCF_LEX_WORD_KEY_GOTO == w0->type);

	printf("%s(),%d\n", __func__, __LINE__);

	scf_block_t* b = parse->ast->current_block;
	while (b && SCF_FUNCTION != b->node.type) {
		b = (scf_block_t*)b->node.parent;
	}

	if (!b) {
		printf("%s(),%d, error: '%s' must be in a function, line: %d\n",
				__func__, __LINE__, w0->text->data, w0->line);
		return -1;
	}
	assert(SCF_FUNCTION == b->node.type);

	scf_lex_word_t* w1 = NULL;
	scf_lex_word_t* w2 = NULL;
	SCF_CHECK_LEX_POP_WORD(w1);
	SCF_CHECK_LEX_POP_WORD(w2);

	if (!scf_lex_is_identity(w1)) {
		printf("%s(),%d, error: '%s' is not a label\n", __func__, __LINE__, w1->text->data);
		return -1;
	}

	if (w2->type != SCF_LEX_WORD_SEMICOLON) {
		printf("%s(),%d, error: '%s' should be ';'\n", __func__, __LINE__, w2->text->data);
		return -1;
	}
	scf_lex_word_free(w2);
	w2 = NULL;

	scf_label_t* l = scf_label_alloc(w1);
	w1 = NULL;
	scf_node_t* nl = scf_node_alloc_label(l);

	scf_node_t* n = scf_node_alloc(w0, SCF_OP_GOTO, NULL);

	scf_node_add_child(n, nl);

	scf_node_add_child((scf_node_t*)parse->ast->current_block, n);
	return 0;
}

int _parse_word(scf_parse_t* parse, scf_lex_word_t* w)
{
	if (SCF_LEX_WORD_SEMICOLON == w->type) {
		scf_lex_word_free(w);
		w = NULL;
		return 0;
	}

	if ((SCF_LEX_WORD_KEY_CHAR <= w->type &&
			SCF_LEX_WORD_KEY_DOUBLE >= w->type)
		|| SCF_LEX_WORD_KEY_STRUCT == w->type
		|| scf_lex_is_identity(w)) {

		int ret = _parse_function(parse, w);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		} else if (1 == ret) {
			printf("%s(),%d, ### parse function ok\n\n", __func__, __LINE__);
			return 0;
		}
		assert(0 == ret);
	}

	if (SCF_LEX_WORD_KEY_CHAR <= w->type &&
			SCF_LEX_WORD_KEY_DOUBLE >= w->type) {

		scf_type_t* t = scf_ast_find_type(parse->ast, w->text->data);
		if (!t) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		printf("%s(),%d, w: %s\n", __func__, __LINE__, w->text->data);

		scf_lex_word_free(w);
		w = NULL;

		scf_scope_t* current_scope = scf_ast_current_scope(parse->ast);

		if (_parse_var_declaration(parse, t, current_scope) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	}

	if (SCF_LEX_WORD_KEY_STRUCT == w->type) {
		scf_lex_push_word(parse->lex, w);
		w = NULL;

		scf_scope_t* current_scope = scf_ast_current_scope(parse->ast);

		if (_parse_struct_define(parse, current_scope) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	}

	if (SCF_LEX_WORD_KEY_IF == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		scf_lex_push_word(parse->lex, w);
		w = NULL;

		scf_node_t* node = NULL;
		if (_parse_if(parse, &node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		printf("%s(),%d, end n: %p\n", __func__, __LINE__, node);
		printf("%s(),%d, end '%s', line: %d\n", __func__, __LINE__, node->w->text->data, node->w->line);
		return 0;
	}
	if (SCF_LEX_WORD_KEY_WHILE == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		scf_lex_push_word(parse->lex, w);
		w = NULL;

		scf_node_t* node = NULL;
		if (_parse_while(parse, &node) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		printf("%s(),%d, end n: %p\n", __func__, __LINE__, node);
		printf("%s(),%d, end '%s', line: %d\n", __func__, __LINE__, node->w->text->data, node->w->line);
		return 0;
	}

	if (SCF_LEX_WORD_KEY_RETURN == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		return _parse_return(parse, w);
	}
	if (SCF_LEX_WORD_KEY_BREAK == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		return _parse_break(parse, w);
	}
	if (SCF_LEX_WORD_KEY_CONTINUE == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		return _parse_continue(parse, w);
	}
	if (SCF_LEX_WORD_KEY_GOTO == w->type) {
		printf("%s(),%d, start '%s', line: %d\n", __func__, __LINE__, w->text->data, w->line);
		return _parse_goto(parse, w);
	}

	if (SCF_LEX_WORD_LB == w->type) {
		scf_lex_push_word(parse->lex, w);
		w = NULL;

		return _parse_block(parse);
	}

	if (scf_lex_is_identity(w)) {
		if (_parse_identity(parse, w) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	}

	printf("%s(),%d, error: w: %d, '%s', w->line: %d, w->pos: %d\n", __func__, __LINE__, w->type, w->text->data, w->line, w->pos);
	return -1;
}

int scf_parse_parse(scf_parse_t* parse)
{
	assert(parse);
	assert(parse->lex);

	while (1) {
		scf_lex_word_t* w = NULL;

		int ret = scf_lex_pop_word(parse->lex, &w);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		if (SCF_LEX_WORD_EOF == w->type) {
			printf("%s(),%d, eof\n", __func__, __LINE__);
			scf_lex_push_word(parse->lex, w);
			w = NULL;
			break;
		}

		if (SCF_LEX_WORD_SPACE == w->type) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			assert(0); // it must not come here, already drop the SPACE in lex
			return -1;
		}

		if (_parse_word(parse, w) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

typedef struct {
	char*			 fname;
	int				 argc;
	scf_variable_t** argv;
} _find_function_arg_t;

static void* _find_function(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	_find_function_arg_t* farg = arg;

	scf_block_t* b = (scf_block_t*)node;

	if (SCF_OP_BLOCK == b->node.type) {
		assert(b->scope);

		scf_function_t* f = scf_scope_find_function(b->scope, farg->fname);

		//printf("%s(),%d, fname: %s, f: %p\n", __func__, __LINE__,
		//		fname, f);

		if (f && farg->argc == f->argv->size) {

		//	printf("%s(),%d, fname: %s, f: %p, argc: %d, f->argv->size: %d\n", __func__, __LINE__,
		//			fname, f, argc, f->argv->size);

			int i;
			for (i = 0; i < farg->argc; i++) {
				scf_variable_t* v = f->argv->data[i];
				if (farg->argv[i]->type != v->type) {
					break;
				}
			}

			if (farg->argc == i) {
				return (void*)f;
			}
		}
	}

	return NULL;
}

int scf_parse_run(scf_parse_t* parse, const char* fname, const int argc, const scf_variable_t** argv, scf_variable_t** pret)
{
	scf_block_t* b = parse->ast->root_block;
	if (!b) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d, fname: %s\n", __func__, __LINE__, fname);

	_find_function_arg_t arg;
	arg.fname	= (char*)fname;
	arg.argc	= argc;
	arg.argv	= (scf_variable_t**)argv;

	scf_function_t* f = (scf_function_t*) scf_node_search_bfs((scf_node_t*)b, _find_function, &arg, NULL);
	if (!f) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d, \n\n", __func__, __LINE__);

	if (scf_function_semantic_analysis(parse->ast, f) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("%s(),%d, \n\n", __func__, __LINE__);
	if (scf_function_to_3ac(parse->ast, f, &parse->code_list_head) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("%s(),%d, run function ok: f: %p, fname: %s\n\n", __func__, __LINE__, f, fname);

#if 1
	scf_3ac_context_t* _3ac_ctx = scf_3ac_context_alloc();
	scf_3ac_filter(_3ac_ctx, NULL, &parse->code_list_head);
#endif

	printf("%s(),%d, \n\n", __func__, __LINE__);
#if 1
	scf_native_context_t* native_ctx = NULL;
	if (scf_native_open(&native_ctx, "x64") < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int ret = scf_native_select_instruction(native_ctx, &parse->code_list_head, f);
	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
#endif
	printf("%s(),%d, \n\n", __func__, __LINE__);
	return 0;
}

