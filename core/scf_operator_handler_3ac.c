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
		return 0;
	}

	scf_handler_data_t* d = data;

	if (0 == node->nb_nodes) {
		if (scf_type_is_var(node->type) && node->var->w) {
			scf_logd("node->var->w->text->data: %s\n", node->var->w->text->data);
		}

		assert(scf_type_is_var(node->type) || SCF_LABEL == node->type);
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->nb_nodes > 0);

	if (!node->op) {
		scf_loge("node->type: %d\n", node->type);
		return -1;
	}

	if (node->_3ac_done)
		return 0;
	node->_3ac_done = 1;

	if (node->w)
		scf_logd("node: %p, node->w->text->data: %s\n", node, node->w->text->data);

	if (SCF_OP_ASSOCIATIVITY_LEFT == node->op->associativity) {
		// left associativity
		scf_logd("left associativity\n");

		int i;
		for (i = 0; i < node->nb_nodes; i++) {
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
		return 0;
	} else {
		// right associativity
		scf_logd("right associativity\n");

		int i;
		for (i = node->nb_nodes - 1; i >= 0; i--) {
			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("op->type: %d, name: '%s'\n", node->op->type, node->op->name);
			return -1;
		}

		int ret = h->func(ast, node->nodes, node->nb_nodes, d);

		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
		return 0;
	}

	scf_loge("\n");
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
		scf_loge("\n");
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

static int _scf_3ac_code_dst(scf_list_t* h, int op_type, scf_node_t* d)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("3ac operator not found\n");
		return -1;
	}

	scf_3ac_operand_t* dst	= scf_3ac_operand_alloc();

	dst->node	= d;

	scf_3ac_code_t* code = scf_3ac_code_alloc();
	code->op	= _3ac_op;
	code->dst	= dst;
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

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_POINTER, parent, nodes[0], nodes[1]);
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

	ret = _scf_3ac_call_extern(d->_3ac_list_head, "scf_create", parent, (scf_node_t**)argv->data, argv->size);

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

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_ARRAY_INDEX, parent, nodes[0], nodes[1]);
}

static int _scf_op_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (0 == nb_nodes) {
		return 0;
	}

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);

	scf_block_t* prev_block = ast->current_block;
	ast->current_block = b;

	int i = 0;
	while (i < nb_nodes) {
		scf_node_t* node = nodes[i];
		scf_operator_t* op = node->op;
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
			scf_label_t* l = node->label;
			assert(node == l->node);

			scf_list_t*     tail = scf_list_tail(d->_3ac_list_head);
			scf_3ac_code_t*	end  = scf_list_data(tail, scf_3ac_code_t, list);

			int j;
			for (j = 0; j < d->branch_ops->_gotos->size; j++) {
				scf_3ac_code_t* c = d->branch_ops->_gotos->data[j];
				if (!c)
					continue;

				assert(l->w);
				assert(c->label->w);

				printf("%s(),%d, l: %s, c->label: %s\n", __func__, __LINE__, l->w->text->data, c->label->w->text->data);

				if (!strcmp(l->w->text->data, c->label->w->text->data)) {
					printf("%s(),%d, j: %d\n", __func__, __LINE__, j);
					c->dst->code = end;
				}
			}
		}

		i++;
	}

	ast->current_block = prev_block;
	printf("%s(),%d, b: %p ok\n\n", __func__, __LINE__, b);
	return 0;
}

static int _scf_op_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_block_t* b = ast->current_block;

	while (b && SCF_FUNCTION  != b->node.type)
		b = (scf_block_t*)b->node.parent;

	if (!b) {
		scf_loge("\n");
		return -1;
	}
	assert(SCF_FUNCTION == b->node.type);

	scf_expr_t* e = nodes[0];

	if (_scf_expr_calculate_internal(ast, e->nodes[0], d) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_RETURN, parent, e->nodes[0]);
}


static int _scf_op_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
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

	scf_3ac_code_t* branch = _scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);

	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	scf_vector_add(d->branch_ops->_breaks, branch);
	return 0;
}

static int _scf_op_continue(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
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

	scf_3ac_code_t* branch = _scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);

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

	scf_3ac_code_t* je   = _scf_branch_ops_code(SCF_OP_3AC_JZ,  NULL, parent);
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
	scf_vector_add(d->branch_ops->_errors, je); // 'je' will re-fill too.

	return 0;
}

static int _scf_op_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (nb_nodes < 2) {
		scf_loge("\n");
		return -1;
	}

	scf_handler_data_t* d = data;

	scf_expr_t* e      = nodes[0];
	scf_node_t* parent = e->parent;

	if (_scf_expr_calculate_internal(ast, e->nodes[0], d) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* result = e->nodes[0]->result;
	assert(result);

	int is_default = 0;
	int jmp_op;
	switch (e->nodes[0]->type) {
		case SCF_OP_EQ:
			jmp_op = SCF_OP_3AC_JNZ;
			break;
		case SCF_OP_NE:
			jmp_op = SCF_OP_3AC_JZ;
			break;
		case SCF_OP_GT:
			jmp_op = SCF_OP_3AC_JLE;
			break;
		case SCF_OP_GE:
			jmp_op = SCF_OP_3AC_JLT;
			break;
		case SCF_OP_LT:
			jmp_op = SCF_OP_3AC_JGE;
			break;
		case SCF_OP_LE:
			jmp_op = SCF_OP_3AC_JGT;
			break;

		default:
			if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, nodes[0]) < 0) {
				scf_loge("\n");
				return -1;
			}
			jmp_op     = SCF_OP_3AC_JZ;
			is_default = 1;
			break;
	};

	if (!is_default) {
		scf_list_t*        l   = scf_list_tail(d->_3ac_list_head);
		scf_3ac_code_t*    c   = scf_list_data(l, scf_3ac_code_t, list);
		scf_3ac_operand_t* dst = c->dst;

		assert(scf_type_is_cmp_operator(c->op->type));

		c->op  = scf_3ac_find_operator(SCF_OP_3AC_CMP);
		c->dst = NULL;

		scf_3ac_operand_free(dst);
		dst = NULL;
	}

	scf_3ac_code_t*	branch	= scf_3ac_code_alloc();
	branch->dst				= scf_3ac_operand_alloc();
	branch->op				= scf_3ac_find_operator(jmp_op);
	scf_loge("jmp op: %d\n", jmp_op);
	scf_loge("branch->op: %p\n", branch->op);

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

	int ret = scf_vector_add(d->branch_ops->_breaks, branch);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
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

	// if the code list is empty, 'start' point to list sentinel,
	// in this status it should NOT be freed by scf_3ac_code_free()

	// we don't know the real start of the while loop here,
	// we only know it's the next of 'start_prev'
	scf_list_t* start_prev = scf_list_tail(d->_3ac_list_head);

	if (_scf_expr_calculate(ast, e, d) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// test if r = 0
	if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, e->nodes[0]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	// if r = 0, je & end the 'while', the 'je destination' should be given later
	scf_3ac_code_t* je = _scf_branch_ops_code(SCF_OP_3AC_JZ, NULL, NULL);
	scf_list_add_tail(d->_3ac_list_head, &je->list);

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

	// add loop when true, jmp to start with no condition
	scf_3ac_code_t*	loop = _scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);
	scf_list_add_tail(d->_3ac_list_head, &loop->list);

	// should get the real start here,
	// because the code may be 'while (1);' & only has the loop itself
	scf_3ac_code_t*	start	= scf_list_data(scf_list_next(start_prev), scf_3ac_code_t, list);
	loop->dst->code = start;

	// set jmp destination for 'continue',
	// it's the 'real' dst & needs not to re-fill
	int i;
	for (i = 0; i < local_branch_ops->_continues->size; i++) {

		scf_3ac_code_t*	branch = local_branch_ops->_continues->data[i];
		assert(branch->dst && !branch->dst->code);

		branch->dst->code = start;
	}

	// get the end, it's NOT the 'real' dst, but the prev of the 'real'
	// we can't get the 'real' here, since the later code is not processed.
	scf_3ac_code_t*	end_prev  = scf_list_data(scf_list_tail(d->_3ac_list_head), scf_3ac_code_t, list);

	// set jmp destination for 'break'
	for (i = 0; i < local_branch_ops->_breaks->size; i++) {

		scf_3ac_code_t*	branch = local_branch_ops->_breaks->data[i];
		assert(branch->dst);

		// 'break' may be delivered by the child loop & the code is set
		if (!branch->dst->code)
			branch->dst->code = end_prev;
	}

	// set the 'je destination' to end the 'while'
	je->dst->code = end_prev;

	if (up_branch_ops) {
		// deliver the 'break' to parent block
		for (i = 0; i < local_branch_ops->_breaks->size; i++) {

			scf_3ac_code_t*	branch = local_branch_ops->_breaks->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_breaks, branch) < 0) {
				scf_loge("deliver 'break' to parent failed\n");
				return -1;
			}
		}

		// je to end while is also added to _breaks
		if (scf_vector_add(up_branch_ops->_breaks, je) < 0) {
			scf_loge("deliver 'je' to parent failed\n");
			return -1;
		}

		// deliver the 'goto' to parent block
		for (i = 0; i < local_branch_ops->_gotos->size; i++) {

			scf_3ac_code_t*	branch = local_branch_ops->_gotos->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_gotos, branch) < 0) {
				scf_loge("deliver 'goto' to parent failed\n");
				return -1;
			}
		}

		// deliver the 'error' to parent block
		for (i = 0; i < local_branch_ops->_errors->size; i++) {

			scf_3ac_code_t*	branch = local_branch_ops->_errors->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_errors, branch) < 0) {
				scf_loge("deliver 'error' to parent failed\n");
				return -1;
			}
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

	ret = _scf_3ac_call_extern(d->_3ac_list_head, "scf_delete", NULL, (scf_node_t**)argv->data, argv->size);
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

	scf_logw("\n");
	// use local_branch_ops, because branch code should NOT jmp over the function block
	if (_scf_op_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}
	scf_logw("\n");

	scf_3ac_code_t* err;
	scf_node_t*     node;
	scf_list_t*     tail;
	scf_list_t*     next;
	int i;
	int ret;

	// re-fill 'break'
	for (i = 0; i < local_branch_ops->_breaks->size; i++) {

		scf_3ac_code_t* c = local_branch_ops->_breaks->data[i];

		if (c->dst->code) {
			next         = scf_list_next(&c->dst->code->list);
			c->dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("'break' has a bug!\n");
			return -1;
		}
	}

	// re-fill 'goto'
	for (i = 0; i < local_branch_ops->_gotos->size; i++) {

		scf_3ac_code_t* c = local_branch_ops->_gotos->data[i];

		if (c->dst->code) {
			next         = scf_list_next(&c->dst->code->list);
			c->dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("all 'goto' should get its label in this function\n");
			return -1;
		}
	}

	if (0 == local_branch_ops->_errors->size)
		goto end;

	// do all the 'error'.
	// first add a 'return', make sure not come here if no error happens.
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_RETURN, NULL);

	for (i = local_branch_ops->_errors->size - 1; i >= 0; i--) {

		err = local_branch_ops->_errors->data[i];
		assert(err);

		if (!err->dst->code) {
			assert(err->error);

			node = err->error;
			err->error = NULL;

			tail = scf_list_tail(d->_3ac_list_head);
			err->dst->code = scf_list_data(tail, scf_3ac_code_t, list);

			ret = _scf_do_error(ast, node, d);
			SCF_CHECK_ERROR(ret < 0, ret, "do error failed\n");
		} else {
			// it is the 'je' when error don't happen, do nothing.
		}
	}

	// pop & return error code
	_scf_3ac_code_dst(d->_3ac_list_head, SCF_OP_3AC_POP, NULL);
	_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_RETURN,  NULL);

	// re-fill 'error'
	for (i = 0; i < local_branch_ops->_errors->size; i++) {

		scf_3ac_code_t* c = local_branch_ops->_errors->data[i];

		if (c->dst->code) {
			next         = scf_list_next(&c->dst->code->list);
			c->dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("'error' has a bug!\n");
			return -1;
		}
	}

end:
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
	scf_logw("\n\n");

	assert(nb_nodes > 0);

	scf_handler_data_t* d = data;

	int ret = _scf_expr_calculate_internal(ast, nodes[0], d);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	scf_function_t* f = v0->func_ptr; 
	assert(f->argv->size == nb_nodes - 1);

	scf_vector_t* argv = scf_vector_alloc();
	scf_vector_add(argv, nodes[0]);

	int i;
	for (i = 1; i < nb_nodes; i++) {
		scf_node_t* arg = nodes[i];

		while (SCF_OP_EXPR == arg->type)
			arg = arg->nodes[0];

		scf_vector_add(argv, arg);
	}

	scf_logw("f: %p, ok\n\n", f);

	scf_node_t* parent = nodes[0]->parent;

	ret = _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_CALL, parent, (scf_node_t**)argv->data, argv->size);

	scf_vector_free(argv);
	argv = NULL;
	return ret;
}

static int _scf_op_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
#if 1
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = nodes[0];
	int ret = _scf_expr_calculate_internal(ast, n, d);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}
#endif
	return 0;
}

static int _scf_op_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_NEG, parent, nodes[0]);
}

static int _scf_op_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_POSITIVE, parent, nodes[0]);
}

static int _scf_op_dereference(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_DEREFERENCE, parent, nodes[0]);
}

static int _scf_op_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ADDRESS_OF, parent, nodes[0]);
}

static int _scf_op_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_TYPE_CAST, parent, nodes[1]);
}

static int _scf_op_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_LOGIC_NOT, parent, nodes[0]);
}

static int _scf_op_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_ADD, parent, nodes[0], nodes[1]);
}

static int _scf_op_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_SUB, parent, nodes[0], nodes[1]);
}

static int _scf_op_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_MUL, parent, nodes[0], nodes[1]);
}

static int _scf_op_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_3(d->_3ac_list_head, SCF_OP_DIV, parent, nodes[0], nodes[1]);
}

static int _scf_op_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	if (_scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ASSIGN, nodes[0], nodes[1]) < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

#define SCF_OP_CMP(name, op_type) \
static int _scf_op_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{\
	assert(2 == nb_nodes);\
	scf_handler_data_t* d  = data;\
	scf_node_t*     parent = nodes[0]->parent; \
	\
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);\
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);\
	if (scf_variable_const(v0)) { \
		if (scf_variable_const(v1)) {\
			scf_loge("result to compare 2 const var should be calculated before\n"); \
			return -EINVAL; \
		} \
		int op_type2 = op_type; \
		switch (op_type) { \
			case SCF_OP_GT: \
				op_type2 = SCF_OP_LT; \
				break; \
			case SCF_OP_GE: \
				op_type2 = SCF_OP_LE; \
				break; \
			case SCF_OP_LE: \
				op_type2 = SCF_OP_GE; \
				break; \
			case SCF_OP_LT: \
				op_type2 = SCF_OP_GT; \
				break; \
		} \
		parent->type = op_type2; \
		SCF_XCHG(nodes[0], nodes[1]); \
		return _scf_3ac_code_3(d->_3ac_list_head, op_type2, parent, nodes[0], nodes[1]); \
	} \
	return _scf_3ac_code_3(d->_3ac_list_head, op_type, parent, nodes[0], nodes[1]); \
}

SCF_OP_CMP(eq, SCF_OP_EQ)
SCF_OP_CMP(ne, SCF_OP_NE)
SCF_OP_CMP(gt, SCF_OP_GT)
SCF_OP_CMP(ge, SCF_OP_GE)
SCF_OP_CMP(lt, SCF_OP_LT)
SCF_OP_CMP(le, SCF_OP_LE)

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
	{{NULL, NULL}, SCF_OP_NE,           -1,     -1, -1, _scf_op_ne},
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

