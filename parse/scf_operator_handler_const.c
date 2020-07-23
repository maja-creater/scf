#include"scf_ast.h"
#include"scf_operator_handler_const.h"
#include"scf_type_cast.h"
#include"scf_calculate.h"

typedef struct {
	scf_variable_t**	pret;

} scf_handler_data_t;

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	if (!node)
		return 0;

	if (0 == node->nb_nodes) {
		assert(scf_type_is_var(node->type) || SCF_LABEL == node->type);

		if (scf_type_is_var(node->type) && node->var->w) {
			scf_logd("w: %s\n", node->var->w->text->data);
		}
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->nb_nodes > 0);

	if (!node->op) {
		node->op = scf_find_base_operator_by_type(node->type);
		if (!node->op) {
			scf_loge("node %p, type: %d, w: %p\n", node, node->type, node->w);
			return -1;
		}
	}

	scf_handler_data_t* d    = data;

	int i;

	if (SCF_OP_ASSOCIATIVITY_LEFT == node->op->associativity) {

		for (i = 0; i < node->nb_nodes; i++) {

			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				goto _error;
			}
		}

		scf_operator_handler_t* h = scf_find_const_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			goto _error;
		}

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			scf_loge("\n");
			goto _error;
		}
	} else {
		for (i = node->nb_nodes - 1; i >= 0; i--) {

			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				goto _error;
			}
		}

		scf_operator_handler_t* h = scf_find_const_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			goto _error;
		}

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			scf_loge("\n");
			goto _error;
		}
	}

	return 0;

_error:
	return -1;
}

static int _scf_op_const_create(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nb_nodes: %d\n", __func__, __LINE__, nb_nodes);

	assert(nb_nodes >= 1);

	scf_handler_data_t* d    = data;

	int ret;
	int i;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0 && SCF_FUNCTION_PTR == v0->type);

	scf_type_t* class  = scf_ast_find_type(ast, v0->w->text->data);
	assert(class);

	for (i = 1; i < nb_nodes; i++) {

		ret     = _scf_expr_calculate_internal(ast, nodes[i], d);
		SCF_CHECK_ERROR(ret < 0, -1, "calculate expr error\n");
	}

	return 0;
}

static int _scf_op_const_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;
	scf_variable_t*     v0 = _scf_operand_get(nodes[0]);
	scf_variable_t*     v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);
	assert(v0->type >= SCF_STRUCT);

	scf_type_t*		t = scf_ast_find_type_type(ast, v1->type);

	return 0;
}

static int _scf_op_const_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (v0->nb_dimentions <= 0) {
		scf_loge("index out\n");
		return -1;
	}

	scf_handler_data_t* d    = data;

	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_op_const_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (0 == nb_nodes)
		return 0;

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);

	scf_block_t* prev_block = ast->current_block;
	ast->current_block = b;

	int i = 0;
	while (i < nb_nodes) {
		scf_node_t*		node = nodes[i];
		scf_operator_t* op   = node->op;
		scf_logd("node: %p, type: %d, i: %d, nb_nodes: %d\n", node, node->type, i, nb_nodes);

		if (SCF_LABEL == node->type) {
			scf_logd("node: %p, w: %s, line:%d\n", node, node->label->w->text->data, node->label->w->line);
		} else if (scf_type_is_var(node->type)) {
			scf_logd("node: %p, w: %s, line:%d\n", node, node->var->w->text->data, node->var->w->line);
		} else if (node->w) {
			scf_logd("node: %p, w: %s, line:%d\n", node, node->w->text->data, node->w->line);
		}

		if (!op) {
			op = scf_find_base_operator_by_type(node->type);
			if (!op) {
				scf_loge("node->type: %d\n", node->type);
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_const_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			scf_loge("\n");
			ast->current_block = prev_block;
			return -1;
		}

		i++;
	}

	ast->current_block = prev_block;
	scf_logw("b: %p ok\n\n", b);
	return 0;
}

static int _scf_op_const_error(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_block_t* b = ast->current_block;

	while (b && SCF_FUNCTION  != b->node.type) {
		b = (scf_block_t*)b->node.parent;
	}

	SCF_CHECK_ERROR(!b, -1, "error statement must in a function\n");
	assert(SCF_FUNCTION == b->node.type);

	assert(nb_nodes >= 2);
	assert(nodes);

	int i;
	for (i = 0; i < nb_nodes; i++) {

		int ret = _scf_expr_calculate_internal(ast, nodes[i], d);

		SCF_CHECK_ERROR(ret < 0, -1, "expr calculate error\n");
	}

	return 0;
}

static int _scf_op_const_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_block_t* b = ast->current_block;

	scf_expr_t* e = nodes[0];

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate_internal(ast, e, &r) < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_op_const_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_continue(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_label(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_goto(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (nb_nodes < 2) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate_internal(ast, e, &r) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int i;
	for (i = 1; i < nb_nodes; i++) {
		scf_node_t*		node = nodes[i];
		scf_operator_t* op   = node->op;

		if (node->w) {
			printf("%s(),%d, node: %p, w: %s, line:%d\n", __func__, __LINE__, node, node->w->text->data, node->w->line);
		}
		if (!op) {
			op = scf_find_base_operator_by_type(node->type);
			if (!op) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_const_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	printf("%s(),%d, ok: \n", __func__, __LINE__);
	return 0;
}

static int _scf_op_const_while(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	scf_logi("e: %p, b: %p, b->node.nodes: %p, b->node.nb_nodes: %d\n",
			e, b, b->node.nodes, b->node.nb_nodes);

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate_internal(ast, e, &r) < 0) {
		scf_loge("\n");
		return -1;
	}

	// while body
	scf_node_t* node = nodes[1];
	scf_operator_t* op = node->op;

	if (node->w)
		scf_logi("node: %p, w: %s, line:%d\n", node, node->w->text->data, node->w->line);

	if (!op) {
		op = scf_find_base_operator_by_type(node->type);
		if (!op) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_operator_handler_t*	h = scf_find_const_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		scf_loge("\n");
		return -1;
	}

	int ret = h->func(ast, node->nodes, node->nb_nodes, d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_logi("ok\n");
	return 0;
}

static int __scf_op_const_call(scf_ast_t* ast, scf_function_t* f, void* data)
{
	scf_logi("f: %p, f->node->w: %s, f->ret->type: %d\n",
			f, f->node.w->text->data, f->ret->type);

	scf_handler_data_t* d = data;

	// save & change the current block
	scf_block_t* prev_block = ast->current_block;
	ast->current_block = (scf_block_t*)f;

	if (_scf_op_const_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	ast->current_block = prev_block;
	return 0;
}

int scf_function_const_opt(scf_ast_t* ast, scf_function_t* f)
{
	scf_logi("f: %p\n", f);

	scf_handler_data_t d = {0};

	int ret = __scf_op_const_call(ast, f, &d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_logi("f: %p ok\n", f);
	return 0;
}

static int _scf_op_const_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n      = nodes[0];
	scf_node_t* parent = nodes[0]->parent;

	while (SCF_OP_EXPR == n->type) {
		n = n->nodes[0];
	}

	n->parent->nodes[0] = NULL;
	scf_node_free(nodes[0]);
	nodes[0]  = n;
	n->parent = parent;

	int ret = _scf_expr_calculate_internal(ast, n, d);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_op_const_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_dereference(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_variable_t* dst    = _scf_operand_get(nodes[0]);
	scf_variable_t* src    = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	if (scf_variable_const(src)) {
#if 0
		scf_type_cast_t* cast = scf_find_base_type_cast(src->type, dst->type);
		if (cast) {
			scf_variable_t* r = NULL;

			int ret = cast->func(ast, &r, src);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
			r->const_flag = 1;

			if (parent->w)
				SCF_XCHG(r->w, parent->w);

			scf_node_free_data(parent);
			parent->type = r->type;
			parent->var  = r;
		}
#endif
	}

	return 0;
}

static int _scf_op_const_sizeof(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_op_const_unary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);

	if (scf_type_is_number(v0->type)) {

		if (!v0->const_flag)
			return 0;

		scf_calculate_t* cal = scf_find_base_calculate(parent->type, v0->type, v0->type);
		if (!cal) {
			scf_loge("type %d not support\n", v0->type);
			return -EINVAL;
		}

		scf_variable_t* r = NULL;
		int ret = cal->func(ast, &r, v0, NULL);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
		r->const_flag = 1;

		SCF_XCHG(r->w, parent->w);

		scf_logi("r: %p\n", r);
		scf_logi("r->data.i64: %ld\n", r->data.i64);

		scf_node_free_data(parent);
		parent->type = r->type;
		parent->var  = r;

	} else {
		scf_loge("type %d not support\n", v0->type);
		return -1;
	}

	return 0;
}

static int _scf_op_const_bit_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_unary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_unary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_binary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (scf_type_is_number(v0->type) && scf_type_is_number(v1->type)) {

		if (!v0->const_flag || !v1->const_flag)
			return 0;

		assert(v0->type == v1->type);

		scf_calculate_t* cal = scf_find_base_calculate(parent->type, v0->type, v1->type);
		if (!cal) {
			scf_loge("type %d, %d not support\n", v0->type, v1->type);
			return -EINVAL;
		}

		scf_variable_t* r = NULL;
		int ret = cal->func(ast, &r, v0, v1);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
		r->const_flag = 1;

		SCF_XCHG(r->w, parent->w);

		scf_logi("r: %p\n", r);
		scf_logi("r->data.i64: %ld\n", r->data.i64);

		scf_node_free_data(parent);
		parent->type = r->type;
		parent->var  = r;

	} else {
		scf_loge("type %d, %d not support\n", v0->type, v1->type);
		return -1;
	}

	return 0;
}

static int _scf_op_const_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_const_cmp(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_eq(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_ne(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_gt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_ge(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_lt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_le(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_logic_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_logic_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_bit_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_const_bit_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_const_binary(ast, nodes, nb_nodes, data);
}

scf_operator_handler_t const_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,			-1, 	-1, -1, _scf_op_const_expr},
	{{NULL, NULL}, SCF_OP_CALL,			-1, 	-1, -1, _scf_op_const_call},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,	-1, 	-1, -1, _scf_op_const_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,      -1,     -1, -1, _scf_op_const_pointer},
	{{NULL, NULL}, SCF_OP_CREATE,       -1,     -1, -1, _scf_op_const_create},

	{{NULL, NULL}, SCF_OP_SIZEOF,       -1,     -1, -1, _scf_op_const_sizeof},
	{{NULL, NULL}, SCF_OP_TYPE_CAST,	-1, 	-1, -1, _scf_op_const_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,	-1, 	-1, -1, _scf_op_const_logic_not},
	{{NULL, NULL}, SCF_OP_BIT_NOT,      -1,     -1, -1, _scf_op_const_bit_not},
	{{NULL, NULL}, SCF_OP_NEG,			-1, 	-1, -1, _scf_op_const_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,		-1, 	-1, -1, _scf_op_const_positive},

	{{NULL, NULL}, SCF_OP_DEREFERENCE,	-1, 	-1, -1, _scf_op_const_dereference},
	{{NULL, NULL}, SCF_OP_ADDRESS_OF,	-1, 	-1, -1, _scf_op_const_address_of},

	{{NULL, NULL}, SCF_OP_MUL,			-1, 	-1, -1, _scf_op_const_mul},
	{{NULL, NULL}, SCF_OP_DIV,			-1, 	-1, -1, _scf_op_const_div},

	{{NULL, NULL}, SCF_OP_ADD,			-1, 	-1, -1, _scf_op_const_add},
	{{NULL, NULL}, SCF_OP_SUB,			-1, 	-1, -1, _scf_op_const_sub},

	{{NULL, NULL}, SCF_OP_BIT_AND,      -1,     -1, -1, _scf_op_const_bit_and},
	{{NULL, NULL}, SCF_OP_BIT_OR,       -1,     -1, -1, _scf_op_const_bit_or},

	{{NULL, NULL}, SCF_OP_EQ,			-1, 	-1, -1, _scf_op_const_eq},
	{{NULL, NULL}, SCF_OP_NE,           -1,     -1, -1, _scf_op_const_ne},
	{{NULL, NULL}, SCF_OP_GT,			-1, 	-1, -1, _scf_op_const_gt},
	{{NULL, NULL}, SCF_OP_LT,			-1, 	-1, -1, _scf_op_const_lt},
	{{NULL, NULL}, SCF_OP_GE,			-1, 	-1, -1, _scf_op_const_ge},
	{{NULL, NULL}, SCF_OP_LE,			-1, 	-1, -1, _scf_op_const_le},

	{{NULL, NULL}, SCF_OP_LOGIC_AND,    -1,     -1, -1, _scf_op_const_logic_and},
	{{NULL, NULL}, SCF_OP_LOGIC_OR,     -1,     -1, -1, _scf_op_const_logic_or},

	{{NULL, NULL}, SCF_OP_ASSIGN,		-1, 	-1, -1, _scf_op_const_assign},


	{{NULL, NULL}, SCF_OP_BLOCK,		-1, 	-1, -1, _scf_op_const_block},
	{{NULL, NULL}, SCF_OP_RETURN,		-1, 	-1, -1, _scf_op_const_return},
	{{NULL, NULL}, SCF_OP_BREAK,		-1, 	-1, -1, _scf_op_const_break},
	{{NULL, NULL}, SCF_OP_CONTINUE,		-1, 	-1, -1, _scf_op_const_continue},
	{{NULL, NULL}, SCF_OP_GOTO,			-1, 	-1, -1, _scf_op_const_goto},
	{{NULL, NULL}, SCF_LABEL,			-1, 	-1, -1, _scf_op_const_label},
	{{NULL, NULL}, SCF_OP_ERROR,        -1,     -1, -1, _scf_op_const_error},

	{{NULL, NULL}, SCF_OP_IF,			-1, 	-1, -1, _scf_op_const_if},
	{{NULL, NULL}, SCF_OP_WHILE,		-1, 	-1, -1, _scf_op_const_while},
};

scf_operator_handler_t* scf_find_const_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type)
{
	int i;
	for (i = 0; i < sizeof(const_operator_handlers) / sizeof(const_operator_handlers[0]); i++) {

		scf_operator_handler_t* h = &const_operator_handlers[i];

		if (type == h->type)
			return h;
	}

	return NULL;
}

