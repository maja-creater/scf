#include"scf_3ac.h"

typedef int (*scf_dag_operator_handler_pt)(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes);

typedef struct scf_dag_operator_s {
	int						type;
	char*					name;
	int						priority;
	int						nb_operands;
	int						associativity;

	scf_dag_operator_handler_pt	func;
} scf_dag_operator_t;

static int _scf_3ac_code_3(scf_list_t* h, int op_type, scf_dag_node_t* d, scf_dag_node_t* n0, scf_dag_node_t* n1)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src1	= scf_3ac_operand_alloc();

	dst->dag_node	= d;
	src0->dag_node	= n0;
	src1->dag_node	= n1;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);
	scf_vector_add(srcs, src1);

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
	code->srcs	= srcs;
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_3ac_code_2(scf_list_t* h, int op_type, scf_dag_node_t* d, scf_dag_node_t* n0)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();

	dst->dag_node	= d;
	src0->dag_node	= n0;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
	code->srcs	= srcs;
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_3ac_code_1(scf_list_t* h, int op_type, scf_dag_node_t* n0)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();

	src0->dag_node = n0;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->srcs	= srcs;
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_dag_op_array_index(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);

	return _scf_3ac_code_3(h, SCF_OP_ARRAY_INDEX, parent, nodes[0], nodes[1]);
}

static int _scf_dag_op_neg(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_NEG, parent, nodes[0]);
}

static int _scf_dag_op_address_of(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_ADDRESS_OF, parent, nodes[0]);
}

static int _scf_dag_op_logic_not(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_LOGIC_NOT, parent, nodes[0]);
}

static int _scf_dag_op_add(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);
	return _scf_3ac_code_3(h, SCF_OP_ADD, parent, nodes[0], nodes[1]);
}

static int _scf_dag_op_sub(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);

	return _scf_3ac_code_3(h, SCF_OP_SUB, parent, nodes[0], nodes[1]);
}

static int _scf_dag_op_mul(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);

	return _scf_3ac_code_3(h, SCF_OP_MUL, parent, nodes[0], nodes[1]);
}

static int _scf_dag_op_div(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);
	return _scf_3ac_code_3(h, SCF_OP_DIV, parent, nodes[0], nodes[1]);
}

static int _scf_dag_op_assign(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_ASSIGN, nodes[0], nodes[1]);
}

static int _scf_dag_op_add_assign(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_ADD_ASSIGN, nodes[0], nodes[1]);
}

#define SCF_OP_CMP(name, operator, op_type) \
static int _scf_dag_op_##name(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{\
	assert(2 == nb_nodes);\
	return _scf_3ac_code_3(h, op_type, parent, nodes[0], nodes[1]); \
}

SCF_OP_CMP(eq, ==, SCF_OP_EQ)
SCF_OP_CMP(gt, >, SCF_OP_GT)
SCF_OP_CMP(lt, <, SCF_OP_LT)

scf_dag_operator_t	dag_operators[] = {
	{SCF_OP_ARRAY_INDEX, 	"[]", 	0, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_array_index},

	{SCF_OP_LOGIC_NOT, 	     "!",   1, 1, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_logic_not},
	{SCF_OP_NEG, 			"-", 	1, 1, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_neg},

	{SCF_OP_ADDRESS_OF, 	"&", 	1, 1, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_address_of},

	{SCF_OP_MUL, 			"*", 	4, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_mul},
	{SCF_OP_DIV, 			"/", 	4, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_div},

	{SCF_OP_ADD, 			"+", 	5, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_add},
	{SCF_OP_SUB, 			"-", 	5, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_sub},

	{SCF_OP_EQ, 			"==", 	7, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_eq},
	{SCF_OP_GT, 			">", 	7, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_gt},
	{SCF_OP_LT, 			"<", 	7, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_lt},
//	{{NULL, NULL}, SCF_OP_GE, 			">=", 	7, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_ge},
//	{{NULL, NULL}, SCF_OP_LE, 			"<=", 	7, 2, SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_le},

	{SCF_OP_ASSIGN, 		"=", 	10, 2, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_assign},
	{SCF_OP_ADD_ASSIGN,     "+=",   10, 2, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_add_assign},
};

scf_dag_operator_t* scf_dag_operator_find(int type)
{
	int i;
	for (i = 0; i < sizeof(dag_operators) / sizeof(dag_operators[0]); i++) {
		if (dag_operators[i].type == type)
			return &(dag_operators[i]);
	}
	return NULL;
}

int	scf_dag_expr_calculate(scf_list_t* h, scf_dag_node_t* node)
{
	if (!node) {
		return 0;
	}

	if (!node->childs || 0 == node->childs->size) {
		if (scf_type_is_var(node->type)) {
//			printf("%s(),%d, node->var->w->text->data: %s\n", __func__, __LINE__, node->var->w->text->data);
		}
		assert(scf_type_is_var(node->type));
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->childs->size > 0);

	scf_dag_operator_t* op = scf_dag_operator_find(node->type);
	if (!op) {
		scf_loge("node->type: %d\n", node->type);
		if (node->var && node->var->w)
			scf_loge("node->var: %s\n", node->var->w->text->data);
		return -1;
	}

	if (SCF_OP_ASSOCIATIVITY_LEFT == op->associativity) {
		// left associativity
		int i;
		for (i = 0; i < node->childs->size; i++) {
			if (scf_dag_expr_calculate(h, node->childs->data[i]) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		if (op->func(h, node, (scf_dag_node_t**)node->childs->data, node->childs->size) < 0) {
			scf_loge("\n");
			return -1;
		}
		return 0;
	} else {
		// right associativity
		int i;
		for (i = node->childs->size - 1; i >= 0; i--) {
			if (scf_dag_expr_calculate(h, node->childs->data[i]) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		if (op->func(h, node, (scf_dag_node_t**)node->childs->data, node->childs->size) < 0) {
			scf_loge("\n");
			return -1;
		}
		return 0;
	}

	scf_loge("\n");
	return -1;
}
