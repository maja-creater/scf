#include"scf_ast.h"
#include"scf_operator_handler.h"
#include"scf_type_cast.h"

typedef struct {
	scf_variable_t**	pret;

} scf_handler_data_t;

scf_operator_handler_t* scf_find_expr_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type);

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	scf_handler_data_t* d    = data;

	if (!node) {
		printf("%s(),%d\n", __func__, __LINE__);
		return 0;
	}

	if (0 == node->nb_nodes) {
		assert(scf_type_is_var(node->type));

		printf("%s(),%d, node->var->w->text->data: %s\n", __func__, __LINE__, node->var->w->text->data);

		if (d->pret)
			*d->pret = scf_variable_clone(node->var);
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->nb_nodes > 0);

	if (!node->op) {
		printf("%s(),%d, error: node %p, w: %p\n", __func__, __LINE__, node, node->w);
		return -1;
	}

	if (node->result) {
		// free the result calculated before
		scf_variable_free(node->result);
		node->result = NULL;
	}

	if (node->w) {
		printf("%s(),%d, node: %p, node->w->text->data: %s\n", __func__, __LINE__, node, node->w->text->data);
	} else {
		printf("%s(),%d, node: %p, node->type: %d\n", __func__, __LINE__, node, node->type);
	}

	scf_variable_t**	pret = d->pret; // save d->pret, and reload it before return

	int i;

	if (SCF_OP_ASSOCIATIVITY_LEFT == node->op->associativity) {
		// left associativity, from 0 to N -1

		for (i = 0; i < node->nb_nodes; i++) {

			d->pret = &(node->nodes[i]->result);

			// recursive calculate every child node
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {

				printf("%s(),%d, error: \n", __func__, __LINE__);
				goto _error;
			}
		}

		// find the handler for this node 
		scf_operator_handler_t* h = scf_find_expr_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			goto _error;
		}

		d->pret = &node->result;

		// calculate this node
		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			goto _error;
		}
	} else {
		// right associativity, from N - 1 to 0

		for (i = node->nb_nodes - 1; i >= 0; i--) {

			d->pret = &(node->nodes[i]->result);

			// recursive calculate every child node
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {

				printf("%s(),%d, error: \n", __func__, __LINE__);
				goto _error;
			}
		}

		// find the handler for this node 
		scf_operator_handler_t* h = scf_find_expr_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			goto _error;
		}

		d->pret = &node->result;

		// calculate this node
		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			goto _error;
		}
	}

	d->pret = pret;

	if (node->result && d->pret) {
		*d->pret = scf_variable_clone(node->result);
	} else {
		printf("\033[31m%s(),%d, warning: node->result: %p, d->pret: %p\033[0m\n", __func__, __LINE__, node->result, d->pret);
	}
	return 0;

_error:
	d->pret = pret;
	return -1;
}

int scf_expr_calculate(scf_ast_t* ast, scf_expr_t* e, scf_variable_t** pret)
{
	assert(e);
	assert(e->nodes);

	scf_node_t* root = e->nodes[0];
	printf("%s(),%d, root: %p\n", __func__, __LINE__, root);

	if (scf_type_is_var(root->type)) {
		printf("%s(),%d, root: %p var: %p\n", __func__, __LINE__, root, root->var);

		if (pret) 
			*pret = scf_variable_clone(root->var);
		return 0;
	}

	scf_handler_data_t d = {0};
	d.pret = &root->result;

	if (_scf_expr_calculate_internal(ast, root, &d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (pret) {
		*pret = scf_variable_clone(root->result);
	}
	return 0;
}

static int _scf_op_expr_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);
	assert(v0->nb_dimentions > 0);

	scf_handler_data_t* d    = data;
	scf_variable_t**	pret = d->pret;

	d->pret = &(nodes[1]->result);
	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);
	d->pret = pret;

	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_variable_t* v1 = _scf_operand_get(nodes[1]);
	assert(SCF_VAR_INT == v1->type);

	int index = v1->data.i;
	if (index < 0 || index >= v0->capacity) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (v0->nb_pointers < 1) {
		scf_loge("\n");
		return -1;
	}

	scf_type_t*		t = scf_ast_find_type_type(ast, v0->type);
	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers - 1, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_expr_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = nodes[0];
	if (n->result) {
		scf_variable_free(n->result);
		n->result = 0;
	}

	scf_variable_t** pret = d->pret;

	d->pret = &n->result;
	int ret = _scf_expr_calculate_internal(ast, n, d);
	d->pret = pret;

	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (scf_type_is_var(n->type)) {
		assert(n->var);
		if (d->pret)
			*d->pret = scf_variable_clone(n->var);
	} else {
		if (n->result && d->pret)
			*d->pret = scf_variable_clone(n->result);
	}
	return 0;
}

static int _scf_op_expr_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_type_is_number(v0->type)) {
		//can add the auto type cast here

		scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);
		assert(t);

		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);
		scf_variable_t* r = scf_variable_alloc(w, t);
		r->data.i = -v0->data.i;
		*d->pret = r;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_op_expr_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_type_is_number(v0->type)) {

		scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);
		assert(t);

		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);
		scf_variable_t* r = scf_variable_alloc(w, t);
		r->data.i = v0->data.i;
		*d->pret = r;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_op_expr_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_type_is_integer(v0->type)) {

		scf_type_t*	t = scf_ast_find_type(ast, "int");
		assert(t);

		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);
		scf_variable_t* r = scf_variable_alloc(w, t);
		r->data.i = !v0->data.i;
		*d->pret = r;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

#define SCF_OP_BINARY(name, op_type, op) \
static int _scf_op_expr_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)\
{\
	assert(2 == nb_nodes);\
	scf_handler_data_t* d = data;\
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);\
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);\
	assert(v0);\
	assert(v1);\
	if (scf_type_is_number(v0->type) && scf_type_is_number(v1->type)) {\
		scf_type_t*		t = scf_ast_find_type_type(ast, v0->type);\
		assert(t);\
		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);\
		scf_variable_t* r = scf_variable_alloc(w, t);\
		printf("%s(),%d, v0: %d\n", __func__, __LINE__, v0->data.i);\
		printf("%s(),%d, v1: %d\n\n", __func__, __LINE__, v1->data.i);\
		r->data.i = v0->data.i op v1->data.i;\
		*d->pret = r;\
	} else {\
		printf("%s(),%d, error: \n", __func__, __LINE__);\
		return -1;\
	}\
	return 0;\
}

SCF_OP_BINARY(add, SCF_OP_ADD, +)
SCF_OP_BINARY(sub, SCF_OP_SUB, -)
SCF_OP_BINARY(mul, SCF_OP_MUL, *)
SCF_OP_BINARY(div, SCF_OP_DIV, /)

static int _scf_op_expr_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nb_nodes: %d\n", __func__, __LINE__, nb_nodes);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	//printf("%s(),%d, v0: %p, v1: %p\n", __func__, __LINE__, v0, v1);

	assert(v0);
	assert(v1);

	assert(v0->type == v1->type); //can add the auto type cast here
	assert(v0->nb_pointers == v1->nb_pointers); //can add the auto type cast here

	if (v0->const_flag) {
		// const variable can't assign
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);
	assert(t);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v1->nb_pointers, v1->func_ptr);
	if (!r)
		return -ENOMEM;

	v0->data.i = v1->data.i;
	r->data.i = v1->data.i;

	printf("%s(),%d, d->pret: %p, %p\n", __func__, __LINE__, d->pret, *d->pret);

	printf("%s(),%d, v0: %d, v1: %d, r: %d\n", __func__, __LINE__, v0->data.i, v1->data.i, r->data.i);

	*d->pret = r;
	return 0;
}

static int _scf_op_expr_eq(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);
	scf_handler_data_t* d = data;
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);
	assert(v0);
	assert(v1);
	if (scf_type_is_number(v0->type) && scf_type_is_number(v1->type)) {

		// float double can't cmp equal
		if (SCF_VAR_DOUBLE == v0->type || SCF_VAR_DOUBLE == v1->type) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_type_t*		t = scf_ast_find_type(ast, "int");
		assert(t);
		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);
		scf_variable_t* r = scf_variable_alloc(w, t);
		r->data.i = (v0->data.i == v1->data.i);
		*d->pret = r;
	} else {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	return 0;
}


#define SCF_OP_CMP(name, op_type, op) \
static int _scf_op_expr_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{\
	printf("%s(),%d\n", __func__, __LINE__);\
	assert(2 == nb_nodes);\
	scf_handler_data_t* d = data;\
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);\
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);\
	assert(v0);\
	assert(v1);\
	if (scf_type_is_number(v0->type) && scf_type_is_number(v0->type)) {\
		scf_type_t*		t = scf_ast_find_type(ast, "int");\
		assert(t);\
		scf_lex_word_t* w = scf_lex_word_clone(nodes[0]->parent->w);\
		scf_variable_t* r = scf_variable_alloc(w, t);\
		r->data.i = (v0->data.i op v1->data.i);\
		*d->pret = r;\
	} else {\
		printf("%s(),%d, error: \n", __func__, __LINE__);\
		return -1;\
	}\
	return 0;\
}

SCF_OP_CMP(gt, SCF_OP_GT, >)
SCF_OP_CMP(ge, SCF_OP_GE, >=)
SCF_OP_CMP(lt, SCF_OP_LT, <)
SCF_OP_CMP(le, SCF_OP_LE, <=)

scf_operator_handler_t expr_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,			-1, 	-1, -1, _scf_op_expr_expr},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,	-1, 	-1, -1, _scf_op_expr_array_index},

	{{NULL, NULL}, SCF_OP_LOGIC_NOT,	-1, 	-1, -1, _scf_op_expr_logic_not},
	{{NULL, NULL}, SCF_OP_NEG,			-1, 	-1, -1, _scf_op_expr_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,		-1, 	-1, -1, _scf_op_expr_positive},

	{{NULL, NULL}, SCF_OP_MUL,			-1, 	-1, -1, _scf_op_expr_mul},
	{{NULL, NULL}, SCF_OP_DIV,			-1, 	-1, -1, _scf_op_expr_div},

	{{NULL, NULL}, SCF_OP_ADD,			-1, 	-1, -1, _scf_op_expr_add},
	{{NULL, NULL}, SCF_OP_SUB,			-1, 	-1, -1, _scf_op_expr_sub},

	{{NULL, NULL}, SCF_OP_EQ,			-1, 	-1, -1, _scf_op_expr_eq},
	{{NULL, NULL}, SCF_OP_GT,			-1, 	-1, -1, _scf_op_expr_gt},
	{{NULL, NULL}, SCF_OP_LT,			-1, 	-1, -1, _scf_op_expr_lt},
	{{NULL, NULL}, SCF_OP_GE,			-1, 	-1, -1, _scf_op_expr_ge},
	{{NULL, NULL}, SCF_OP_LE,			-1, 	-1, -1, _scf_op_expr_le},


	{{NULL, NULL}, SCF_OP_ASSIGN,		-1, 	-1, -1, _scf_op_expr_assign},
};

scf_operator_handler_t* scf_find_expr_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type)
{
	int i;
	for (i = 0; i < sizeof(expr_operator_handlers) / sizeof(expr_operator_handlers[0]); i++) {

		scf_operator_handler_t* h = &expr_operator_handlers[i];

		if (type == h->type)
			return h;
	}

	return NULL;
}

