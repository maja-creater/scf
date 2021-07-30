#include"scf_ast.h"
#include"scf_operator_handler_const.h"
#include"scf_operator_handler_semantic.h"
#include"scf_type_cast.h"
#include"scf_calculate.h"

typedef struct {
	scf_variable_t**  pret;
} scf_handler_data_t;

scf_operator_handler_t* scf_find_expr_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type);

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	if (!node)
		return 0;

	if (0 == node->nb_nodes) {
		assert(scf_type_is_var(node->type));

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

		scf_operator_handler_t* h = scf_find_expr_operator_handler(node->op->type, -1, -1, -1);
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

		scf_operator_handler_t* h = scf_find_expr_operator_handler(node->op->type, -1, -1, -1);
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

static int _scf_op_expr_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);
	return 0;
}

static int _scf_op_expr_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_node_t* parent = nodes[0]->parent;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	if (!scf_variable_const(v1)) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!v0->const_literal_flag) {
		scf_loge("\n");
		return -EINVAL;
	}

	return 0;
}

static int _scf_op_expr_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n      = nodes[0];
	scf_node_t* parent = nodes[0]->parent;

	while (SCF_OP_EXPR == n->type)
		n = n->nodes[0];

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

static int _scf_op_expr_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_expr_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_variable_t* dst    = _scf_operand_get(nodes[0]);
	scf_variable_t* src    = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	if (scf_variable_const(src)) {

		int dst_type = dst->type;

		if (dst->nb_pointers + dst->nb_dimentions > 0)
			dst_type = SCF_VAR_UINTPTR;

		scf_type_cast_t* cast = scf_find_base_type_cast(src->type, dst_type);
		if (cast) {
			scf_variable_t* r = NULL;

			int ret = cast->func(ast, &r, src);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
			r->const_flag  = 1;
			r->type        = dst->type;
			r->nb_pointers = scf_variable_nb_pointers(dst);
			r->const_literal_flag = 1;

			if (parent->w)
				SCF_XCHG(r->w, parent->w);

			scf_loge("parent: %p\n", parent);
			scf_node_free_data(parent);
			parent->type = r->type;
			parent->var  = r;
		}

		return 0;
	} else if (scf_variable_const_string(src)) {

		assert(src == nodes[1]->var);

		scf_variable_t* v = scf_variable_ref(src);
		assert(v);

		scf_node_free_data(parent);

		scf_loge("parent->result: %p, parent: %p, v->type: %d\n", parent->result, parent, v->type);
		parent->type = v->type;
		parent->var  = v;

		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_expr_sizeof(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return -1;
}

static int _scf_op_expr_unary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);

	if (!v0->const_flag)
		return -1;

	if (scf_type_is_number(v0->type)) {

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

		scf_node_free_data(parent);
		parent->type = r->type;
		parent->var  = r;

	} else {
		scf_loge("type %d not support\n", v0->type);
		return -1;
	}

	return 0;
}

static int _scf_op_expr_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_unary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_bit_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_unary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_unary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_binary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (scf_type_is_number(v0->type) && scf_type_is_number(v1->type)) {

		if (!scf_variable_const(v0) || !scf_variable_const(v1)) {
			scf_loge("\n");
			return -1;
		}

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

		scf_node_free_data(parent);
		parent->type = r->type;
		parent->var  = r;

	} else {
		scf_loge("type %d, %d not support\n", v0->type, v1->type);
		return -1;
	}

	return 0;
}

static int _scf_op_expr_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_mod(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _shift_check_const(scf_node_t** nodes, int nb_nodes)
{
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	if (scf_variable_const(v1)) {

		if (v1->data.i < 0) {
			scf_loge("shift count %d < 0\n", v1->data.i);
			return -EINVAL;
		}

		if (v1->data.i >= v0->size << 3) {
			scf_loge("shift count %d >= type bits: %d\n", v1->data.i, v0->size << 3);
			return -EINVAL;
		}
	}

	return 0;
}

static int _scf_op_expr_shl(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	int ret = _shift_check_const(nodes, nb_nodes);
	if (ret < 0)
		return ret;

	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_shr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	int ret = _shift_check_const(nodes, nb_nodes);
	if (ret < 0)
		return ret;

	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _expr_left(scf_member_t** pm, scf_node_t* left)
{
	scf_variable_t*  idx;
	scf_member_t*    m;

	m = scf_member_alloc(NULL);
	if (!m)
		return -ENOMEM;

	while (left) {

		if (SCF_OP_EXPR == left->type) {

		} else if (SCF_OP_ARRAY_INDEX == left->type) {

			idx = _scf_operand_get(left->nodes[1]);

			if (scf_member_add_index(m, NULL, idx->data.i) < 0)
				return -1;

		} else if (SCF_OP_POINTER == left->type) {

			idx = _scf_operand_get(left->nodes[1]);

			if (scf_member_add_index(m, idx, 0) < 0)
				return -1;
		} else
			break;

		left = left->nodes[0];
	}

	assert(scf_type_is_var(left->type));

	m->base = _scf_operand_get(left);

	*pm = m;
	return 0;
}

static int _expr_right(scf_member_t** pm, scf_node_t* right)
{
	scf_variable_t*  idx;
	scf_member_t*    m;

	m = scf_member_alloc(NULL);
	if (!m)
		return -ENOMEM;

	while (right) {

		if (SCF_OP_EXPR == right->type)
			right = right->nodes[0];

		else if (SCF_OP_ASSIGN == right->type)
			right = right->nodes[1];

		else if (SCF_OP_ADDRESS_OF == right->type) {

			if (scf_member_add_index(m, NULL, -SCF_OP_ADDRESS_OF) < 0)
				return -1;

			right = right->nodes[0];

		} else if (SCF_OP_ARRAY_INDEX == right->type) {

			idx = _scf_operand_get(right->nodes[1]);
			assert(idx->data.i >= 0);

			if (scf_member_add_index(m, NULL, idx->data.i) < 0)
				return -1;

			right = right->nodes[0];

		} else if (SCF_OP_POINTER == right->type) {

			idx = _scf_operand_get(right->nodes[1]);

			if (scf_member_add_index(m, idx, 0) < 0)
				return -1;

			right = right->nodes[0];
		} else
			break;
	}

	assert(scf_type_is_var(right->type));

	m->base = _scf_operand_get(right);

	*pm = m;
	return 0;
}

static int _expr_init_const(scf_member_t* m0, scf_variable_t* v1)
{
	scf_variable_t* base = m0->base;

	if (!m0->indexes) {
		memcpy(&base->data, &v1->data, v1->size);
		return 0;
	}

	assert(scf_variable_is_array (base)
		|| scf_variable_is_struct(base));

	int size   = scf_variable_size(base);
	int offset = scf_member_offset(m0);

	assert(offset < size);

	if (!base->data.p) {
		base ->data.p = calloc(1, size);
		if (!base->data.p)
			return -ENOMEM;
	}

	memcpy(base->data.p + offset, &v1->data, v1->size);
	return 0;
}

static int _expr_init_address(scf_ast_t* ast, scf_member_t* m0, scf_member_t* m1)
{
	int ret = scf_vector_add_unique(ast->global_consts, m1->base);
	if (ret < 0)
		return ret;

	scf_ast_rela_t* r = calloc(1, sizeof(scf_ast_rela_t));
	if (!r)
		return -ENOMEM;
	r->ref = m0;
	r->obj = m1;

	if (scf_vector_add(ast->global_relas, r) < 0) {
		free(r);
		return -ENOMEM;
	}

	return 0;
}

static int _scf_op_expr_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_variable_t* v0  = _scf_operand_get(nodes[0]);
	scf_variable_t* v1  = _scf_operand_get(nodes[1]);
	scf_member_t*   m0  = NULL;
	scf_member_t*   m1  = NULL;

	scf_logw("v0->type: %d, v1->type: %d, SCF_VAR_U8: %d\n",
			v0->type, v1->type, SCF_VAR_U8);

	if (!scf_variable_const_string(v1))
		assert(v0->type == v1->type);

	assert(v0->size == v1->size);

	if (scf_variable_is_array(v1) || scf_variable_is_struct(v1)) {
		scf_loge("\n");
		return -1;
	}

	if (_expr_left(&m0, nodes[0]) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (_expr_right(&m1, nodes[1]) < 0) {
		scf_loge("\n");
		scf_member_free(m0);
		return -1;
	}

	int ret = -1;

	if (!m1->indexes) {

		if (scf_variable_const_string(m1->base)) {

			ret = _expr_init_address(ast, m0, m1);

		} else if (scf_variable_const(m1->base)) {

			if (SCF_FUNCTION_PTR == m1->base->type) {

				ret = _expr_init_address(ast, m0, m1);
			} else {
				ret = _expr_init_const(m0, m1->base);

				scf_member_free(m0);
				scf_member_free(m1);
				return ret;
			}
		} else {
			scf_variable_t* v = m1->base;
			scf_loge("v: %d/%s\n", v->w->line, v->w->text->data);
			return -1;
		}
	} else {
		scf_index_t* idx;

		assert(m1->indexes->size >= 1);

		idx = m1->indexes->data[0];

		assert(-SCF_OP_ADDRESS_OF == idx->index);

		assert(0 == scf_vector_del(m1->indexes, idx));

		free(idx);
		idx = NULL;

		ret = _expr_init_address(ast, m0, m1);
	}

	if (ret < 0) {
		scf_loge("\n");
		scf_member_free(m0);
		scf_member_free(m1);
	}
	return ret;
}

static int _scf_op_expr_cmp(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_eq(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_ne(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_gt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_ge(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_lt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_le(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_logic_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_logic_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_bit_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_expr_bit_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_expr_binary(ast, nodes, nb_nodes, data);
}

scf_operator_handler_t expr_operator_handlers[] = {

	{{NULL, NULL}, SCF_OP_EXPR,			  -1, 	-1, -1, _scf_op_expr_expr},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,	  -1, 	-1, -1, _scf_op_expr_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,        -1,   -1, -1, _scf_op_expr_pointer},

	{{NULL, NULL}, SCF_OP_SIZEOF,         -1,   -1, -1, _scf_op_expr_sizeof},
	{{NULL, NULL}, SCF_OP_TYPE_CAST,	  -1, 	-1, -1, _scf_op_expr_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,	  -1, 	-1, -1, _scf_op_expr_logic_not},
	{{NULL, NULL}, SCF_OP_BIT_NOT,        -1,   -1, -1, _scf_op_expr_bit_not},
	{{NULL, NULL}, SCF_OP_NEG,			  -1, 	-1, -1, _scf_op_expr_neg},

	{{NULL, NULL}, SCF_OP_ADDRESS_OF,	  -1, 	-1, -1, _scf_op_expr_address_of},

	{{NULL, NULL}, SCF_OP_MUL,			  -1, 	-1, -1, _scf_op_expr_mul},
	{{NULL, NULL}, SCF_OP_DIV,			  -1, 	-1, -1, _scf_op_expr_div},
	{{NULL, NULL}, SCF_OP_MOD,            -1,   -1, -1, _scf_op_expr_mod},

	{{NULL, NULL}, SCF_OP_ADD,			  -1, 	-1, -1, _scf_op_expr_add},
	{{NULL, NULL}, SCF_OP_SUB,			  -1, 	-1, -1, _scf_op_expr_sub},

	{{NULL, NULL}, SCF_OP_SHL,            -1,   -1, -1, _scf_op_expr_shl},
	{{NULL, NULL}, SCF_OP_SHR,            -1,   -1, -1, _scf_op_expr_shr},

	{{NULL, NULL}, SCF_OP_BIT_AND,        -1,   -1, -1, _scf_op_expr_bit_and},
	{{NULL, NULL}, SCF_OP_BIT_OR,         -1,   -1, -1, _scf_op_expr_bit_or},

	{{NULL, NULL}, SCF_OP_EQ,			  -1, 	-1, -1, _scf_op_expr_eq},
	{{NULL, NULL}, SCF_OP_NE,             -1,   -1, -1, _scf_op_expr_ne},
	{{NULL, NULL}, SCF_OP_GT,			  -1, 	-1, -1, _scf_op_expr_gt},
	{{NULL, NULL}, SCF_OP_LT,			  -1, 	-1, -1, _scf_op_expr_lt},
	{{NULL, NULL}, SCF_OP_GE,			  -1, 	-1, -1, _scf_op_expr_ge},
	{{NULL, NULL}, SCF_OP_LE,			  -1,   -1, -1, _scf_op_expr_le},

	{{NULL, NULL}, SCF_OP_LOGIC_AND,      -1,   -1, -1, _scf_op_expr_logic_and},
	{{NULL, NULL}, SCF_OP_LOGIC_OR,       -1,   -1, -1, _scf_op_expr_logic_or},

	{{NULL, NULL}, SCF_OP_ASSIGN,         -1,   -1, -1, _scf_op_expr_assign},
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

int scf_expr_calculate(scf_ast_t* ast, scf_expr_t* e, scf_variable_t** pret)
{
	if (!e || !e->nodes || e->nb_nodes <= 0)
		return -1;

	if (scf_expr_semantic_analysis(ast, e) < 0)
		return -1;

	scf_handler_data_t d = {0};
	scf_variable_t*    v;

	if (!scf_type_is_var(e->nodes[0]->type)) {

		if (_scf_expr_calculate_internal(ast, e->nodes[0], &d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	v = _scf_operand_get(e->nodes[0]);

	if (!scf_variable_const(v) && SCF_OP_ASSIGN != e->nodes[0]->type) {
		scf_loge("\n");
		return -1;
	}

	if (pret)
		*pret = scf_variable_ref(v);

	return 0;
}
