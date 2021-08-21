#include"scf_3ac.h"

typedef struct scf_dag_operator_s {
	int type;
	int associativity;

	int (*func)(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes);

} scf_dag_operator_t;

static int _scf_3ac_code_N(scf_list_t* h, int op_type, scf_dag_node_t* d, scf_dag_node_t** nodes, int nb_nodes)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_vector_t* srcs = scf_vector_alloc();

	int i;
	for (i = 0; i < nb_nodes; i++) {
		scf_3ac_operand_t* src = scf_3ac_operand_alloc();
		src->dag_node = nodes[i];
		scf_vector_add(srcs, src);
	}

	scf_3ac_code_t* c = scf_3ac_code_alloc();
	c->op	= _3ac_op;
	c->srcs	= srcs;

	if (d) {
		scf_3ac_operand_t* dst = scf_3ac_operand_alloc();

		dst->dag_node = d;

		c->dsts = scf_vector_alloc();

		if (scf_vector_add(c->dsts, dst) < 0)
			return -ENOMEM;
	}

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _scf_3ac_code_3(scf_list_t* h, int op_type, scf_dag_node_t* d, scf_dag_node_t* n0, scf_dag_node_t* n1)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src1	= scf_3ac_operand_alloc();

	src0->dag_node	= n0;
	src1->dag_node	= n1;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);
	scf_vector_add(srcs, src1);

	scf_3ac_code_t* c = scf_3ac_code_alloc();
	c->op	= _3ac_op;
	c->srcs	= srcs;

	if (d) {
		scf_3ac_operand_t* dst = scf_3ac_operand_alloc();

		dst->dag_node = d;

		c->dsts = scf_vector_alloc();

		if (scf_vector_add(c->dsts, dst) < 0)
			return -ENOMEM;
	}

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _scf_3ac_code_2(scf_list_t* h, int op_type, scf_dag_node_t* d, scf_dag_node_t* n0)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();

	src0->dag_node	= n0;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);

	scf_3ac_code_t* c = scf_3ac_code_alloc();
	c->op	= _3ac_op;
	c->srcs	= srcs;

	if (d) {
		scf_3ac_operand_t* dst = scf_3ac_operand_alloc();

		dst->dag_node = d;

		c->dsts = scf_vector_alloc();

		if (scf_vector_add(c->dsts, dst) < 0)
			return -ENOMEM;
	}

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _scf_3ac_code_dst(scf_list_t* h, int op_type, scf_dag_node_t* d)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_code_t* c = scf_3ac_code_alloc();
	c->op = _3ac_op;

	if (d) {
		scf_3ac_operand_t* dst = scf_3ac_operand_alloc();

		dst->dag_node = d;

		c->dsts = scf_vector_alloc();

		if (scf_vector_add(c->dsts, dst) < 0)
			return -ENOMEM;
	}

	scf_list_add_tail(h, &c->list);
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
	assert(3 == nb_nodes);
	return _scf_3ac_code_N(h, SCF_OP_ARRAY_INDEX, parent, nodes, nb_nodes);
}

static int _scf_dag_op_pointer(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);
	return _scf_3ac_code_N(h, SCF_OP_POINTER, parent, nodes, nb_nodes);
}

static int _scf_dag_op_neg(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_2(h, SCF_OP_NEG, parent, nodes[0]);
}

static int _scf_dag_op_bit_not(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_2(h, SCF_OP_BIT_NOT, parent, nodes[0]);
}

static int _scf_dag_op_address_of(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	switch (nb_nodes) {
		case 1:
			return _scf_3ac_code_2(h, SCF_OP_ADDRESS_OF, parent, nodes[0]);
			break;

		case 2:
			return _scf_3ac_code_N(h, SCF_OP_3AC_ADDRESS_OF_POINTER, parent, nodes, nb_nodes);
			break;

		case 3:
			return _scf_3ac_code_N(h, SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX, parent, nodes, nb_nodes);
			break;
		default:
			break;
	};

	scf_loge("\n");
	return -1;
}

static int _scf_dag_op_dereference(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_DEREFERENCE, parent, nodes[0]);
}

static int _scf_dag_op_type_cast(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_2(h, SCF_OP_TYPE_CAST, parent, nodes[0]);
}

static int _scf_dag_op_logic_not(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);

	return _scf_3ac_code_2(h, SCF_OP_LOGIC_NOT, parent, nodes[0]);
}

static int _scf_dag_op_inc(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_1(h, SCF_OP_3AC_INC, nodes[0]);
}

static int _scf_dag_op_dec(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_1(h, SCF_OP_3AC_DEC, nodes[0]);
}

#define SCF_DAG_BINARY(name, op) \
static int _scf_dag_op_##name(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{ \
	assert(2 == nb_nodes); \
	return _scf_3ac_code_3(h, SCF_OP_##op, parent, nodes[0], nodes[1]); \
}
SCF_DAG_BINARY(add, ADD)
SCF_DAG_BINARY(sub, SUB)

SCF_DAG_BINARY(shl, SHL)
SCF_DAG_BINARY(shr, SHR)

SCF_DAG_BINARY(and, BIT_AND)
SCF_DAG_BINARY(or,  BIT_OR)

SCF_DAG_BINARY(mul, MUL)
SCF_DAG_BINARY(div, DIV)
SCF_DAG_BINARY(mod, MOD)


#define SCF_DAG_BINARY_ASSIGN(name, op) \
static int _scf_dag_op_##name(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{ \
	assert(2 == nb_nodes); \
	return _scf_3ac_code_2(h, SCF_OP_##op, nodes[0], nodes[1]); \
}
SCF_DAG_BINARY_ASSIGN(assign,     ASSIGN)
SCF_DAG_BINARY_ASSIGN(add_assign, ADD_ASSIGN)
SCF_DAG_BINARY_ASSIGN(sub_assign, SUB_ASSIGN)

SCF_DAG_BINARY_ASSIGN(mul_assign, MUL_ASSIGN)
SCF_DAG_BINARY_ASSIGN(div_assign, DIV_ASSIGN)
SCF_DAG_BINARY_ASSIGN(mod_assign, MOD_ASSIGN)

SCF_DAG_BINARY_ASSIGN(shl_assign, SHL_ASSIGN)
SCF_DAG_BINARY_ASSIGN(shr_assign, SHR_ASSIGN)

SCF_DAG_BINARY_ASSIGN(and_assign, AND_ASSIGN)
SCF_DAG_BINARY_ASSIGN(or_assign,  OR_ASSIGN)

static int _scf_dag_op_assign_array_index(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(4 == nb_nodes);
	return _scf_3ac_code_N(h, SCF_OP_3AC_ASSIGN_ARRAY_INDEX, NULL, nodes, nb_nodes);
}

#define SCF_DAG_ASSIGN_POINTER(name, op) \
static int _scf_dag_op_##name##_pointer(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{ \
	return _scf_3ac_code_N(h, SCF_OP_3AC_##op##_POINTER, NULL, nodes, nb_nodes); \
}
SCF_DAG_ASSIGN_POINTER(assign,     ASSIGN);
SCF_DAG_ASSIGN_POINTER(add_assign, ADD_ASSIGN);
SCF_DAG_ASSIGN_POINTER(sub_assign, SUB_ASSIGN);
SCF_DAG_ASSIGN_POINTER(and_assign, AND_ASSIGN);
SCF_DAG_ASSIGN_POINTER(or_assign,  OR_ASSIGN);

static int _scf_dag_op_cmp(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(2 == nb_nodes);
	return _scf_3ac_code_3(h, SCF_OP_3AC_CMP, NULL, nodes[0], nodes[1]);
}

static int _scf_dag_op_teq(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes)
{
	assert(1 == nb_nodes);
	return _scf_3ac_code_1(h, SCF_OP_3AC_TEQ, nodes[0]);
}

#define SCF_OP_SETCC(name, op_type) \
static int _scf_dag_op_##name(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{ \
	assert(1 == nb_nodes); \
	return _scf_3ac_code_dst(h, op_type, nodes[0]); \
}
SCF_OP_SETCC(setz,  SCF_OP_3AC_SETZ);
SCF_OP_SETCC(setnz, SCF_OP_3AC_SETNZ);
SCF_OP_SETCC(setlt, SCF_OP_3AC_SETLT);
SCF_OP_SETCC(setle, SCF_OP_3AC_SETLE);
SCF_OP_SETCC(setgt, SCF_OP_3AC_SETGT);
SCF_OP_SETCC(setge, SCF_OP_3AC_SETGE);

#define SCF_OP_CMP(name, operator, op_type) \
static int _scf_dag_op_##name(scf_list_t* h, scf_dag_node_t* parent, scf_dag_node_t** nodes, int nb_nodes) \
{\
	assert(2 == nb_nodes);\
	return _scf_3ac_code_3(h, op_type, parent, nodes[0], nodes[1]); \
}

SCF_OP_CMP(eq, ==, SCF_OP_EQ)
SCF_OP_CMP(gt, >, SCF_OP_GT)
SCF_OP_CMP(lt, <, SCF_OP_LT)

scf_dag_operator_t	dag_operators[] =
{
	{SCF_OP_ARRAY_INDEX,    SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_array_index},
	{SCF_OP_POINTER,        SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_pointer},

	{SCF_OP_BIT_NOT,        SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_bit_not},
	{SCF_OP_LOGIC_NOT, 	    SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_logic_not},
	{SCF_OP_NEG,            SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_neg},

	{SCF_OP_3AC_INC,        SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_inc},
	{SCF_OP_3AC_DEC,        SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_dec},

	{SCF_OP_ADDRESS_OF,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_address_of},
	{SCF_OP_DEREFERENCE,    SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_dereference},
	{SCF_OP_TYPE_CAST,      SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_type_cast},

	{SCF_OP_MUL,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_mul},
	{SCF_OP_DIV,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_div},
	{SCF_OP_MOD,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_mod},

	{SCF_OP_ADD,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_add},
	{SCF_OP_SUB,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_sub},

	{SCF_OP_SHL,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_shl},
	{SCF_OP_SHR,            SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_shr},

	{SCF_OP_BIT_AND,        SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_and},
	{SCF_OP_BIT_OR,         SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_or},

	{SCF_OP_EQ,             SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_eq},
	{SCF_OP_GT,             SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_gt},
	{SCF_OP_LT,             SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_lt},

	{SCF_OP_3AC_CMP,        SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_cmp},
	{SCF_OP_3AC_TEQ,        SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_teq},

	{SCF_OP_3AC_SETZ,       SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setz},
	{SCF_OP_3AC_SETNZ,      SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setnz},
	{SCF_OP_3AC_SETLT,      SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setlt},
	{SCF_OP_3AC_SETLE,      SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setle},
	{SCF_OP_3AC_SETGT,      SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setgt},
	{SCF_OP_3AC_SETGE,      SCF_OP_ASSOCIATIVITY_LEFT, _scf_dag_op_setge},

	{SCF_OP_ASSIGN,         SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_assign},
	{SCF_OP_ADD_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_add_assign},
	{SCF_OP_SUB_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_sub_assign},

	{SCF_OP_MUL_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_mul_assign},
	{SCF_OP_DIV_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_div_assign},
	{SCF_OP_MOD_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_mod_assign},

	{SCF_OP_SHL_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_shl_assign},
	{SCF_OP_SHR_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_shr_assign},

	{SCF_OP_AND_ASSIGN,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_and_assign},
	{SCF_OP_OR_ASSIGN,      SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_or_assign},

	{SCF_OP_3AC_ASSIGN_ARRAY_INDEX, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_assign_array_index},

	{SCF_OP_3AC_ASSIGN_POINTER,     SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_assign_pointer},
	{SCF_OP_3AC_ADD_ASSIGN_POINTER, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_add_assign_pointer},
	{SCF_OP_3AC_SUB_ASSIGN_POINTER, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_sub_assign_pointer},
	{SCF_OP_3AC_AND_ASSIGN_POINTER, SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_and_assign_pointer},
	{SCF_OP_3AC_OR_ASSIGN_POINTER,  SCF_OP_ASSOCIATIVITY_RIGHT, _scf_dag_op_or_assign_pointer},
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

		scf_variable_t* v = node->var;
#if 0
		if (v) {
			if (v->w)
				scf_logw("node: %p, v_%d_%d/%s\n", node, v->w->line, v->w->pos, v->w->text->data);
			else
				scf_logw("node: %p, v_%#lx\n", node, 0xffff & (uintptr_t)v);
		} else
			scf_logw("node: %p\n", node);
#endif
		//assert(scf_type_is_var(node->type));
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->childs->size > 0);
#if 1
	if (node->done)
		return 0;
	node->done = 1;
#endif
	scf_dag_operator_t* op = scf_dag_operator_find(node->type);
	if (!op) {
		scf_loge("node->type: %d, SCF_OP_3AC_ASSIGN_POINTER: %d\n", node->type, SCF_OP_3AC_ASSIGN_POINTER);
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
