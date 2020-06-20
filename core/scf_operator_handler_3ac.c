#include"scf_ast.h"
#include"scf_operator_handler.h"
#include"scf_3ac.h"

typedef struct {
	scf_vector_t*	_breaks;
	scf_vector_t*	_continues;
	scf_vector_t*	_gotos;
	scf_vector_t*	_errors;
} scf_branch_ops_t;

typedef struct {
	scf_branch_ops_t*	branch_ops;
	scf_list_t*			_3ac_list_head;

} scf_handler_data_t;

scf_branch_ops_t* scf_branch_ops_alloc()
{
	scf_branch_ops_t* branch_ops = calloc(1, sizeof(scf_branch_ops_t));
	assert(branch_ops);

	branch_ops->_breaks		= scf_vector_alloc();
	branch_ops->_continues	= scf_vector_alloc();
	branch_ops->_gotos		= scf_vector_alloc();
	branch_ops->_errors     = scf_vector_alloc();

	return branch_ops;
}

void scf_branch_ops_free(scf_branch_ops_t* branch_ops)
{
	if (branch_ops) {
		scf_vector_free(branch_ops->_breaks);
		scf_vector_free(branch_ops->_continues);
		scf_vector_free(branch_ops->_gotos);
		scf_vector_free(branch_ops->_errors);

		free(branch_ops);
		branch_ops = NULL;
	}
}

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	if (!node) {
		printf("%s(),%d\n", __func__, __LINE__);
		return 0;
	}

	scf_handler_data_t* d = data;

	if (0 == node->nb_nodes) {
		if (scf_type_is_var(node->type)) {
			printf("%s(),%d, node->var->w->text->data: %s\n", __func__, __LINE__, node->var->w->text->data);
		}
		assert(scf_type_is_var(node->type) || SCF_LABEL == node->type);
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->nb_nodes > 0);

	if (!node->op) {
		printf("%s(),%d, error: node %p, w: %p\n", __func__, __LINE__, node, node->w);
		return -1;
	}

	if (node->w)
		printf("%s(),%d, node: %p, node->w->text->data: %s\n", __func__, __LINE__, node, node->w->text->data);

	if (SCF_OP_ASSOCIATIVITY_LEFT == node->op->associativity) {
		// left associativity
		printf("%s(),%d, left associativity\n", __func__, __LINE__);

		int i;
		for (i = 0; i < node->nb_nodes; i++) {
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	} else {
		// right associativity
		printf("%s(),%d, right associativity\n", __func__, __LINE__);

		int i;
		for (i = node->nb_nodes - 1; i >= 0; i--) {
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}
		}
		printf("%s(),%d, right associativity\n", __func__, __LINE__);

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: op->type: %d, name: '%s'\n", __func__, __LINE__, node->op->type, node->op->name);
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_expr_calculate(scf_ast_t* ast, scf_expr_t* e, void* data)
{
	scf_handler_data_t* d = data;

	assert(e);
	assert(e->nodes);

	scf_node_t* root = e->nodes[0];

	if (scf_type_is_var(root->type)) {
		printf("%s(),%d, root: %p var: %p\n", __func__, __LINE__, root, root->var);
		return 0;
	}

	if (_scf_expr_calculate_internal(ast, root, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int _scf_3ac_call_extern(scf_list_t* h, const char* fname, scf_node_t* d, scf_node_t** nodes, int nb_nodes)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(SCF_OP_3AC_CALL_EXTERN);
	if (!_3ac_op) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	dst->node	= d;

	scf_vector_t* srcs = scf_vector_alloc();
	int i;
	for (i = 0; i < nb_nodes; i++) {
		scf_3ac_operand_t* src = scf_3ac_operand_alloc();
		src->node = nodes[i];
		scf_vector_add(srcs, src);
	}

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
	code->srcs	= srcs;
	code->extern_fname = scf_string_cstr(fname);
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_3ac_code_N(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t** nodes, int nb_nodes)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	dst->node	= d;

	scf_vector_t* srcs = scf_vector_alloc();
	int i;
	for (i = 0; i < nb_nodes; i++) {
		scf_3ac_operand_t* src = scf_3ac_operand_alloc();
		src->node = nodes[i];
		scf_vector_add(srcs, src);
	}

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
	code->srcs	= srcs;
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_3ac_code_3(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t* n0, scf_node_t* n1)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src1	= scf_3ac_operand_alloc();

	dst->node	= d;
	src0->node	= n0;
	src1->node	= n1;

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

static int _scf_3ac_code_2(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t* n0)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();
	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();

	dst->node	= d;
	src0->node	= n0;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
	code->srcs	= srcs;
	scf_list_add_tail(h, &code->list);
	return 0;
}

static int _scf_3ac_code_1(scf_list_t* h, int op_type, scf_node_t* n0)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();

	src0->node = n0;

	scf_vector_t* srcs = scf_vector_alloc();
	scf_vector_add(srcs, src0);

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->srcs	= srcs;

	scf_list_add_tail(h, &code->list);
	return 0;
}

static scf_3ac_code_t* _scf_branch_ops_code(int type, scf_label_t* l, scf_node_t* err)
{
	scf_3ac_code_t* branch = scf_3ac_code_alloc();

	branch->label	= l;
	branch->error   = err;
	branch->dst		= scf_3ac_operand_alloc();
	branch->op		= scf_3ac_find_operator(type);
	assert(branch->op);

	return branch;
}

static int _scf_op_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_POINTER, parent, nodes[0], nodes[1]);
	}

	return 0;
}

static int _scf_op_create(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(nb_nodes >= 1);

	scf_handler_data_t* d      = data;
	scf_vector_t*       argv   = scf_vector_alloc();
	scf_node_t*         parent = nodes[0]->parent;

	int i;
	int ret;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0 && SCF_FUNCTION_PTR == v0->type);

	scf_type_t* class  = scf_ast_find_type(ast, v0->w->text->data);
	assert(class);

	scf_type_t* t = scf_ast_find_type_type(ast, SCF_VAR_INT);
	assert(t);

	scf_variable_t* var_size = SCF_VAR_ALLOC_BY_TYPE(v0->w, t, 1, 0, NULL);
	SCF_CHECK_ERROR(!var_size, -1, "node of obj size alloc failed");
	var_size->const_literal_flag = 1;

	scf_node_t* node_size = scf_node_alloc(NULL, var_size->type, var_size);
	SCF_CHECK_ERROR(!node_size, -1, "node of obj size alloc failed");

	scf_vector_add(argv, node_size);
	scf_vector_add(argv, nodes[0]);

	for (i = 1; i < nb_nodes; i++) {
		ret = _scf_expr_calculate_internal(ast, nodes[i], d);
		SCF_CHECK_ERROR(ret < 0, -1, "calculate expr error\n");
		scf_vector_add(argv, nodes[i]);
	}

	ret = 0;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		ret = _scf_3ac_call_extern(d->_3ac_list_head, "scf_create", parent, (scf_node_t**)argv->data, argv->size);
	}

	scf_vector_free(argv);
	argv = NULL;

	return ret;
}

static int _scf_op_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);
	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_ARRAY_INDEX, parent, nodes[0], nodes[1]);
	}

	return 0;
}

static int _scf_op_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);
	if (0 == nb_nodes) {
		printf("%s(),%d\n", __func__, __LINE__);
		return 0;
	}

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);
	printf("%s(),%d, b: %p, nodes: %p, %d\n", __func__, __LINE__, b, b->node.nodes, b->node.nb_nodes);

	scf_block_t* prev_block = ast->current_block;
	ast->current_block = b;

	int i = 0;
	while (i < nb_nodes) {
		scf_node_t* node = nodes[i];
		scf_operator_t* op = node->op;
		printf("%s(),%d, node: %p, type: %d, i: %d, nb_nodes: %d\n", __func__, __LINE__, node, node->type, i, nb_nodes);

		if (SCF_LABEL == node->type) {
			printf("%s(),%d, node: %p, w: %s, line:%d\n", __func__, __LINE__, node, node->label->w->text->data, node->label->w->line);
		} else if (scf_type_is_var(node->type)) {
			printf("%s(),%d, node: %p, w: %s, line:%d\n", __func__, __LINE__, node, node->var->w->text->data, node->var->w->line);
		} else if (node->w) {
			printf("%s(),%d, node: %p, w: %s, line:%d\n", __func__, __LINE__, node, node->w->text->data, node->w->line);
		}

		if (!op) {
			op = scf_find_base_operator_by_type(node->type);
			if (!op) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			ast->current_block = prev_block;
			return -1;
		}

		// for goto
		if (SCF_LABEL == node->type) {
			printf("%s(),%d\n", __func__, __LINE__);
			scf_label_t* l = node->label;
			assert(node == l->node);

			scf_list_t*     tail = scf_list_tail(d->_3ac_list_head);
			scf_3ac_code_t*	end  = scf_list_data(tail, scf_3ac_code_t, list);

			int j;
			for (j = 0; j < d->branch_ops->_gotos->size; j++) {
			printf("%s(),%d, j: %d\n", __func__, __LINE__, j);
				scf_3ac_code_t* c = d->branch_ops->_gotos->data[j];
				if (!c)
					continue;

				assert(l->w);
				assert(c->label->w);

				printf("%s(),%d, l: %s, c->label: %s\n", __func__, __LINE__, l->w->text->data, c->label->w->text->data);

				if (!strcmp(l->w->text->data, c->label->w->text->data)) {
					printf("%s(),%d, j: %d\n", __func__, __LINE__, j);
					c->dst->code = end;
					d->branch_ops->_gotos->data[j] = NULL;
				}
			}
			printf("%s(),%d\n", __func__, __LINE__);
		}

		i++;
	}

	ast->current_block = prev_block;
	printf("%s(),%d, b: %p ok\n\n", __func__, __LINE__, b);
	return 0;
}

static int _scf_op_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_block_t* b = ast->current_block;

	while (b && SCF_FUNCTION  != b->node.type) {
		b = (scf_block_t*)b->node.parent;
	}

	if (!b) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	assert(SCF_FUNCTION == b->node.type);

	assert(1 == nb_nodes);
	assert(nodes);

	scf_function_t* f = (scf_function_t*)b;

	scf_expr_t* e = nodes[0];

	if (_scf_expr_calculate(ast, e, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_RETURN, parent, e->nodes[0]);
	}

	return 0;
}


static int _scf_op_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, ######################### nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = (scf_node_t*)ast->current_block;

	while (n && (SCF_OP_WHILE != n->type && SCF_OP_FOR != n->type)) {
		n = n->parent;
	}

	if (!n) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);

	scf_node_t* parent = n->parent;
	assert(parent);

	scf_3ac_code_t* branch = _scf_branch_ops_code(SCF_OP_3AC_JNE, NULL, NULL);

	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	scf_vector_add(d->branch_ops->_breaks, branch);
	return 0;
}

static int _scf_op_continue(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, ################ nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = (scf_node_t*)ast->current_block;

	while (n && (SCF_OP_WHILE != n->type && SCF_OP_FOR != n->type)) {
		n = n->parent;
	}

	if (!n) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);

	scf_3ac_code_t* branch = _scf_branch_ops_code(SCF_OP_3AC_JNE, NULL, NULL);

	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	scf_vector_add(d->branch_ops->_continues, branch);
	return 0;
}

static int _scf_op_label(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	return 0;
}

static int _scf_op_goto(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, ################ nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* nl = nodes[0];
	assert(SCF_LABEL == nl->type);

	scf_label_t* l = nl->label;
	assert(l->w);

	scf_3ac_code_t* branch = _scf_branch_ops_code(SCF_OP_GOTO, l, NULL);

	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	scf_vector_add(d->branch_ops->_gotos, branch);
	return 0;
}

static int _scf_op_error(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, ################ nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);
	assert(nb_nodes >= 3);

	scf_handler_data_t* d = data;

	scf_expr_t* e      = nodes[0];
	scf_node_t* parent = e->parent;
	int ret;
	int i;

	if (_scf_expr_calculate(ast, e, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, e->nodes[0]) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_code_t* je   = _scf_branch_ops_code(SCF_OP_3AC_JE,  NULL, parent);
	scf_3ac_code_t* jmp  = _scf_branch_ops_code(SCF_OP_GOTO,    NULL, parent);

	scf_list_add_tail(d->_3ac_list_head, &je->list);

	// print log if needs
	if (nb_nodes > 3) {
		scf_vector_t* argv = scf_vector_alloc();

		for (i = 3; i < nb_nodes; i++) {

			e   = nodes[i];
			ret = _scf_expr_calculate(ast, e, d);
			SCF_CHECK_ERROR(ret < 0, ret, "expr calculate failed\n");

			scf_vector_add(argv, e->nodes[0]);
		}

		ret = _scf_3ac_call_extern(d->_3ac_list_head, "printf", parent, (scf_node_t**)argv->data, argv->size);
		scf_vector_free(argv);
		argv = NULL;
		SCF_CHECK_ERROR(ret < 0, ret, "expr calculate failed\n");
	}

	// calculate error code to return
	e   = nodes[2];
	ret = _scf_expr_calculate(ast, e, d);
	SCF_CHECK_ERROR(ret < 0, ret, "expr calculate failed\n");

	// push error code to stack, and we will pop & return it when exit this function
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_PUSH, e->nodes[0]);

	// set the 'je' destination (code when no error happens)
	je->dst->code = jmp;

	// add 'jmp' to code list & error branches, it will be re-filled when this function end
	scf_list_add_tail(d->_3ac_list_head, &jmp->list);
	scf_vector_add(d->branch_ops->_errors, jmp);

	return 0;
}

static int _scf_op_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);

	if (nb_nodes < 2) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	printf("%s(),%d, e: %p, b: %p, b->node.nodes: %p, b->node.nb_nodes: %d\n",
			__func__, __LINE__, e, b, b->node.nodes, b->node.nb_nodes);

	if (_scf_expr_calculate(ast, e, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;

		scf_variable_t* result = e->nodes[0]->result;
		assert(result);

		if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, e->nodes[0]) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_3ac_code_t*	branch	= scf_3ac_code_alloc();
		branch->dst				= scf_3ac_operand_alloc();
		branch->op				= scf_3ac_find_operator(SCF_OP_3AC_JE);
		assert(branch->op);
		scf_list_add_tail(d->_3ac_list_head, &branch->list);

		int i;
		for (i = 1; i < nb_nodes; i++) {
			scf_node_t* node = nodes[i];
			scf_operator_t* op = node->op;
			printf("%s(),%d, node: %p, type: %d, i: %d\n", __func__, __LINE__,
					node, node->type, i);

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

			scf_operator_handler_t* h = scf_find_3ac_operator_handler(op->type, -1, -1, -1);
			if (!h) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}

			if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
				printf("%s(),%d, error: \n", __func__, __LINE__);
				return -1;
			}

			if (1 == i) {
				scf_list_t* l = scf_list_tail(d->_3ac_list_head);

				branch->dst->code = scf_list_data(l, scf_3ac_code_t, list);
			}
		}
	}

	printf("%s(),%d, ok: \n", __func__, __LINE__);
	return 0;
}

static int _scf_op_while(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nodes: %p, nb_nodes: %d\n", __func__, __LINE__, nodes, nb_nodes);

	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	printf("%s(),%d, e: %p, b: %p, b->node.nodes: %p, b->node.nb_nodes: %d\n",
			__func__, __LINE__, e, b, b->node.nodes, b->node.nb_nodes);

	// if the code list is empty, 'start' point to 'code_list_head',
	// in this status it should NOT be freed by scf_3ac_code_free()
	scf_list_t*		tail	= scf_list_tail(d->_3ac_list_head);
	scf_3ac_code_t*	start	= scf_list_data(tail, scf_3ac_code_t, list);

	if (_scf_expr_calculate(ast, e, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// test if r = 0
	if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, e->nodes[0]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// if r = 0, jmp & end the 'while', the 'jmp destination' should be given later
	scf_3ac_code_t* branch	= _scf_branch_ops_code(SCF_OP_3AC_JE, NULL, NULL);
	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	// while body
	scf_node_t*     node = nodes[1];
	scf_operator_t* op   = node->op;
	printf("%s(),%d, node: %p, type: %d\n", __func__, __LINE__, node, node->type);

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

	scf_operator_handler_t*	h = scf_find_3ac_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_branch_ops_t* local_branch_ops = scf_branch_ops_alloc();
	scf_branch_ops_t* up_branch_ops    = d->branch_ops;
	d->branch_ops                      = local_branch_ops;

	if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// add loop when true
	scf_3ac_code_t*	loop = _scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);
	loop->dst->code = start;
	scf_list_add_tail(d->_3ac_list_head, &loop->list);

	// set jmp destination for 'continue'
	int i;
	for (i = 0; i < local_branch_ops->_continues->size; i++) {
		scf_3ac_code_t*	branch = local_branch_ops->_continues->data[i];
		assert(branch->dst);
		branch->dst->code = start;
	}

	tail = scf_list_tail(d->_3ac_list_head);
	scf_3ac_code_t*	end  = scf_list_data(tail, scf_3ac_code_t, list);

	// set jmp destination for 'break'
	for (i = 0; i < local_branch_ops->_breaks->size; i++) {
		scf_3ac_code_t*	branch = local_branch_ops->_breaks->data[i];
		assert(branch->dst);
		branch->dst->code = start;
	}

	// set the 'jmp destination' to end the 'while' 
	branch->dst->code = end;

	if (up_branch_ops) {
		// deliver the 'goto' to parent block
		for (i = 0; i < local_branch_ops->_gotos->size; i++) {

			scf_3ac_code_t*	branch = local_branch_ops->_gotos->data[i];
			if (branch) {
				scf_vector_add(up_branch_ops->_gotos, branch);
				local_branch_ops->_gotos->data[i] = NULL;
			}
		}

		// deliver the 'error' to parent block
		for (i = 0; i < local_branch_ops->_errors->size; i++) {

			scf_3ac_code_t*	branch = local_branch_ops->_errors->data[i];
			assert(branch);

			scf_vector_add(up_branch_ops->_errors, branch);
			local_branch_ops->_errors->data[i] = NULL;
		}
	}

	d->branch_ops    = up_branch_ops;
	scf_branch_ops_free(local_branch_ops);
	local_branch_ops = NULL;
	printf("%s(),%d, ok: \n", __func__, __LINE__);
	return 0;
}

static int _scf_do_error(scf_ast_t* ast, scf_node_t* err, scf_handler_data_t* d)
{
	assert(err->nb_nodes >= 3);

	scf_expr_t* e = err->nodes[1];

	int i;
	int ret = _scf_expr_calculate(ast, e, d);
	SCF_CHECK_ERROR(ret < 0, ret, "expr calculate failed\n");

	scf_variable_t* r = e->result;
	assert(r);

	scf_function_t* f = NULL;

	if (r->type >= SCF_STRUCT && 1 == r->nb_pointers) {
		scf_type_t* t = scf_ast_find_type_type(ast, r->type);
		assert(t);

		f = scf_scope_find_function(t->scope, "release");
		scf_logw("type '%s' has no release() function, use default\n", t->name->data);
	}

	scf_vector_t* argv = scf_vector_alloc();
	scf_vector_add(argv, f);
	scf_vector_add(argv, e->nodes[0]);

	ret = _scf_3ac_call_extern(d->_3ac_list_head, "scf_delete", err, (scf_node_t**)argv->data, argv->size);
	scf_vector_free(argv);
	argv = NULL;
	SCF_CHECK_ERROR(ret < 0, ret, "call scf_delete failed\n");

	return 0;
}

static int __scf_op_call(scf_ast_t* ast, scf_function_t* f, void* data)
{
	printf("%s(),%d, f: %p, f->node->w: %s, f->ret->type: %d\n", __func__, __LINE__,
			f, f->node.w->text->data, f->ret->type);

	scf_handler_data_t* d = data;

	// save & change the current block
	scf_block_t* prev_block = ast->current_block;
	ast->current_block = (scf_block_t*)f;

	scf_branch_ops_t* local_branch_ops = scf_branch_ops_alloc();
	scf_branch_ops_t* tmp_branch_ops   = d->branch_ops;
	d->branch_ops 					   = local_branch_ops;

	// use local_branch_ops, because branch code should NOT jmp over the function block
	if (_scf_op_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		printf("%s(),%d, error:\n", __func__, __LINE__);
		return -1;
	}

	scf_3ac_code_t* err;
	scf_node_t*     node;
	scf_list_t*     tail;
	int i;
	int ret;

	// all the 'goto' should be done in function body block
	for (i = 0; i < local_branch_ops->_gotos->size; i++) {
		if (local_branch_ops->_gotos->data[i]) {
			printf("%s(),%d, error:\n", __func__, __LINE__);
			return -1;
		}
	}

	// do all the 'error'.
	// first add a 'return', make sure not come here if no error happens.
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_RETURN, NULL);

	for (i = local_branch_ops->_errors->size - 1; i >= 0; i--) {

		err = local_branch_ops->_errors->data[i];
		assert(err);
		assert(err->error);

		node = err->error;
		err->error = NULL;

		tail = scf_list_tail(d->_3ac_list_head);
		err->dst->code = scf_list_data(tail, scf_3ac_code_t, list);

		ret = _scf_do_error(ast, node, d);
		SCF_CHECK_ERROR(ret < 0, ret, "do error failed\n");
	}

	// pop & return error code
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_POP, NULL);
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_RETURN,  NULL);

	scf_branch_ops_free(local_branch_ops);
	local_branch_ops = NULL;
	d->branch_ops    = tmp_branch_ops;

	printf("%s(),%d\n", __func__, __LINE__);
	ast->current_block = prev_block;
	return 0;
}

int scf_function_to_3ac(scf_ast_t* ast, scf_function_t* f, scf_list_t* _3ac_list_head)
{
	printf("%s(),%d, f: %p\n", __func__, __LINE__, f);

	scf_handler_data_t d = {0};
	d._3ac_list_head  	 = _3ac_list_head;

	printf("\033[31m%s(),%d, _3ac_list_head: %p\033[0m\n", __func__, __LINE__, _3ac_list_head);
	printf("\033[31m%s(),%d, d: %p, d->_3ac_list_head: %p\033[0m\n", __func__, __LINE__, &d, d._3ac_list_head);

	int ret = __scf_op_call(ast, f, &d);

	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("%s(),%d, f: %p ok\n", __func__, __LINE__, f);
	return 0;
}

static int _scf_op_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(nb_nodes > 0);

	scf_handler_data_t* d = data;

	if (!nodes[0]->op) {
		nodes[0]->op = scf_find_base_operator_by_type(nodes[0]->type);
		if (!nodes[0]->op) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	scf_operator_handler_t* h = scf_find_3ac_operator_handler(nodes[0]->op->type, -1, -1, -1);
	if (!h) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (h->func(ast, nodes[0]->nodes, nodes[0]->nb_nodes, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (SCF_FUNCTION_PTR != nodes[0]->result->type || !(nodes[0]->result->func_ptr)) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_function_t* f = nodes[0]->result->func_ptr; 
	assert(f->argv->size == nb_nodes - 1);

	printf("%s(),%d, f: %p\n", __func__, __LINE__, f);

	scf_vector_t* argv = scf_vector_alloc();
	scf_vector_add(argv, f);

	int i;
	for (i = 1; i < nb_nodes; i++) {
		scf_vector_add(argv, nodes[i]);
	}

	printf("%s(),%d, f: %p ok\n", __func__, __LINE__, f);

	int ret = 0;
	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		ret = _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_CALL, parent, (scf_node_t**)argv->data, argv->size);
	}

	scf_vector_free(argv);
	argv = NULL;
	return ret;
}

static int _scf_op_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = nodes[0];

	int ret = _scf_expr_calculate_internal(ast, n, d);
	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int _scf_op_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_NEG, parent, nodes[0]);
	}
	return 0;
}

static int _scf_op_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_POSITIVE, parent, nodes[0]);
	}
	return 0;
}

static int _scf_op_dereference(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_DEREFERENCE, parent, nodes[0]);
	}
	return 0;
}

static int _scf_op_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ADDRESS_OF, parent, nodes[0]);
	}

	return 0;
}

static int _scf_op_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_TYPE_CAST, parent, nodes[1]);
	}

	return 0;
}

static int _scf_op_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_LOGIC_NOT, parent, nodes[0]);
	}

	return 0;
}

static int _scf_op_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_ADD, parent, nodes[0], nodes[1]);
	}
	return 0;
}

static int _scf_op_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_SUB, parent, nodes[0], nodes[1]);
	}
	return 0;
}

static int _scf_op_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_MUL, parent, nodes[0], nodes[1]);
	}
	return 0;
}

static int _scf_op_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;
		return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_DIV, parent, nodes[0], nodes[1]);
	}
	return 0;
}

static int _scf_op_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	if (!parent->_3ac_already) {
		parent->_3ac_already = 1;

		scf_node_t* origin = nodes[1];
		while (SCF_OP_ASSIGN == origin->type) {
			assert(2 == origin->nb_nodes);
			assert(origin->nodes[1]);
			origin = origin->nodes[1];
		}

		if (_scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ASSIGN, nodes[0], origin) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

#define SCF_OP_CMP(name, operator, op_type) \
static int _scf_op_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{\
	printf("%s(),%d\n", __func__, __LINE__);\
	assert(2 == nb_nodes);\
	scf_handler_data_t* d = data;\
	scf_node_t* parent = nodes[0]->parent;\
	if (!parent->_3ac_already) { \
		parent->_3ac_already = 1; \
		return _scf_3ac_code_3(d->_3ac_list_head, op_type, parent, nodes[0], nodes[1]); \
	} \
	return 0;\
}

SCF_OP_CMP(eq, ==, SCF_OP_EQ)
SCF_OP_CMP(gt, >, SCF_OP_GT)
SCF_OP_CMP(ge, >=, SCF_OP_GE)
SCF_OP_CMP(lt, <, SCF_OP_LT)
SCF_OP_CMP(le, <=, SCF_OP_LE)

scf_operator_handler_t _3ac_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,			-1, 	-1, -1, _scf_op_expr},
	{{NULL, NULL}, SCF_OP_CALL,			-1, 	-1, -1, _scf_op_call},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,	-1, 	-1, -1, _scf_op_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,      -1,     -1, -1, _scf_op_pointer},
	{{NULL, NULL}, SCF_OP_CREATE,       -1,     -1, -1, _scf_op_create},

	{{NULL, NULL}, SCF_OP_TYPE_CAST,	-1, 	-1, -1, _scf_op_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,	-1, 	-1, -1, _scf_op_logic_not},
	{{NULL, NULL}, SCF_OP_NEG,			-1, 	-1, -1, _scf_op_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,		-1, 	-1, -1, _scf_op_positive},

	{{NULL, NULL}, SCF_OP_DEREFERENCE,	-1, 	-1, -1, _scf_op_dereference},
	{{NULL, NULL}, SCF_OP_ADDRESS_OF,	-1, 	-1, -1, _scf_op_address_of},

	{{NULL, NULL}, SCF_OP_MUL,			-1, 	-1, -1, _scf_op_mul},
	{{NULL, NULL}, SCF_OP_DIV,			-1, 	-1, -1, _scf_op_div},

	{{NULL, NULL}, SCF_OP_ADD,			-1, 	-1, -1, _scf_op_add},
	{{NULL, NULL}, SCF_OP_SUB,			-1, 	-1, -1, _scf_op_sub},

	{{NULL, NULL}, SCF_OP_EQ,			-1, 	-1, -1, _scf_op_eq},
	{{NULL, NULL}, SCF_OP_GT,			-1, 	-1, -1, _scf_op_gt},
	{{NULL, NULL}, SCF_OP_LT,			-1, 	-1, -1, _scf_op_lt},
	{{NULL, NULL}, SCF_OP_GE,			-1, 	-1, -1, _scf_op_ge},
	{{NULL, NULL}, SCF_OP_LE,			-1, 	-1, -1, _scf_op_le},


	{{NULL, NULL}, SCF_OP_ASSIGN,		-1, 	-1, -1, _scf_op_assign},


	{{NULL, NULL}, SCF_OP_BLOCK,		-1, 	-1, -1, _scf_op_block},
	{{NULL, NULL}, SCF_OP_RETURN,		-1, 	-1, -1, _scf_op_return},
	{{NULL, NULL}, SCF_OP_BREAK,		-1, 	-1, -1, _scf_op_break},
	{{NULL, NULL}, SCF_OP_CONTINUE,		-1, 	-1, -1, _scf_op_continue},
	{{NULL, NULL}, SCF_OP_GOTO,			-1, 	-1, -1, _scf_op_goto},
	{{NULL, NULL}, SCF_LABEL,			-1, 	-1, -1, _scf_op_label},
	{{NULL, NULL}, SCF_OP_ERROR,        -1,     -1, -1, _scf_op_error},

	{{NULL, NULL}, SCF_OP_IF,			-1, 	-1, -1, _scf_op_if},
	{{NULL, NULL}, SCF_OP_WHILE,		-1, 	-1, -1, _scf_op_while},
};

scf_operator_handler_t* scf_find_3ac_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type)
{
	int i;
	for (i = 0; i < sizeof(_3ac_operator_handlers) / sizeof(_3ac_operator_handlers[0]); i++) {

		scf_operator_handler_t* h = &_3ac_operator_handlers[i];

		if (type == h->type)
			return h;
	}

	return NULL;
}

