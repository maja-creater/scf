#include"scf_ast.h"
#include"scf_operator_handler.h"
#include"scf_3ac.h"

typedef struct {
	scf_vector_t*	_breaks;
	scf_vector_t*	_continues;
	scf_vector_t*	_gotos;
	scf_vector_t*	_labels;
	scf_vector_t*	_errors;
	scf_vector_t*	_ends;
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
	branch_ops->_labels     = scf_vector_alloc();
	branch_ops->_errors     = scf_vector_alloc();
	branch_ops->_ends       = scf_vector_alloc();

	return branch_ops;
}

void scf_branch_ops_free(scf_branch_ops_t* branch_ops)
{
	if (branch_ops) {
		scf_vector_free(branch_ops->_breaks);
		scf_vector_free(branch_ops->_continues);
		scf_vector_free(branch_ops->_gotos);
		scf_vector_free(branch_ops->_labels);
		scf_vector_free(branch_ops->_errors);
		scf_vector_free(branch_ops->_ends);

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

		if (SCF_OP_LOGIC_AND != node->op->type && SCF_OP_LOGIC_OR != node->op->type) {
			int i;
			for (i = 0; i < node->nb_nodes; i++) {
				if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
					scf_loge("\n");
					return -1;
				}
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

		if (!scf_type_is_assign(node->op->type)
				&& SCF_OP_ADDRESS_OF != node->op->type) {
			int i;
			for (i = node->nb_nodes - 1; i >= 0; i--) {
				if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
					scf_loge("\n");
					return -1;
				}
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
		return 0;
	}

	if (_scf_expr_calculate_internal(ast, root, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_3ac_call_extern(scf_list_t* h, const char* fname, scf_node_t* d, scf_node_t** nodes, int nb_nodes)
{
	scf_loge("\n");
	return -1;
}

static int _scf_3ac_code_NN(scf_list_t* h, int op_type, scf_node_t** dsts, int nb_dsts, scf_node_t** srcs, int nb_srcs)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* operand;
	scf_3ac_code_t*    c;

	scf_vector_t*      vsrc = NULL;
	scf_vector_t*      vdst = NULL;

	int i;

	if (srcs) {
		vsrc = scf_vector_alloc();
		for (i = 0; i < nb_srcs; i++) {

			operand = scf_3ac_operand_alloc();

			operand->node = srcs[i];
			scf_vector_add(vsrc, operand);
		}
	}

	if (dsts) {
		vdst = scf_vector_alloc();
		for (i = 0; i < nb_dsts; i++) {

			operand = scf_3ac_operand_alloc();

			operand->node = dsts[i];
			scf_vector_add(vdst, operand);
		}
	}

	c       = scf_3ac_code_alloc();
	c->op   = _3ac_op;
	c->dsts = vdst;
	c->srcs = vsrc;
	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _scf_3ac_code_N(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t** nodes, int nb_nodes)
{
	return _scf_3ac_code_NN(h, op_type, &d, 1, nodes, nb_nodes);
}

static int _scf_3ac_code_3(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t* n0, scf_node_t* n1)
{
	scf_node_t* srcs[2] = {n0, n1};

	return _scf_3ac_code_NN(h, op_type, &d, 1, srcs, 2);
}

static int _scf_3ac_code_2(scf_list_t* h, int op_type, scf_node_t* d, scf_node_t* n0)
{
	return _scf_3ac_code_NN(h, op_type, &d, 1, &n0, 1);
}

static int _scf_3ac_code_1(scf_list_t* h, int op_type, scf_node_t* n0)
{
	return _scf_3ac_code_NN(h, op_type, NULL, 0, &n0, 1);
}

static int _scf_3ac_code_dst(scf_list_t* h, int op_type, scf_node_t* d)
{
	return _scf_3ac_code_NN(h, op_type, &d, 1, NULL, 0);
}

static int _scf_3ac_code_srcN(scf_list_t* h, int op_type, scf_node_t** nodes, int nb_nodes)
{
	return _scf_3ac_code_NN(h, op_type, NULL, 0, nodes, nb_nodes);
}

static int _scf_op_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;
	scf_variable_t*     v;

	scf_node_t* parent = nodes[0]->parent;

	v = _scf_operand_get(parent);
	v->local_flag = 1;

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

static int _scf_op_array_scale(scf_ast_t* ast, scf_node_t* parent, scf_node_t** pscale)
{
	scf_variable_t* v_member;
	scf_variable_t* v_scale;
	scf_type_t*     t_scale;
	scf_node_t*     n_scale;

	v_member = _scf_operand_get(parent);

	int size = scf_variable_size(v_member);
	assert(size > 0);

	t_scale = scf_ast_find_type_type(ast, SCF_VAR_INTPTR);
	v_scale = SCF_VAR_ALLOC_BY_TYPE(NULL, t_scale, 0, 0, NULL);
	if (!v_scale)
		return -ENOMEM;
	v_scale->data.i = size;

	n_scale = scf_node_alloc(NULL, v_scale->type, v_scale);
	if (!n_scale)
		return -ENOMEM;

	*pscale = n_scale;
	return 0;
}

static int _scf_op_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d   = data;

	scf_node_t*     parent  = nodes[0]->parent;
	scf_node_t*     n_index = NULL;
	scf_node_t*     n_scale = NULL;
	scf_node_t*     srcs[3];
	scf_variable_t* v;

	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = _scf_op_array_scale(ast, parent, &n_scale);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	n_index = nodes[1];
	while (SCF_OP_EXPR == n_index->type)
		n_index = n_index->nodes[0];

	srcs[0] = nodes[0];
	srcs[1] = n_index;
	srcs[2] = n_scale;

	v = _scf_operand_get(parent);
	v->local_flag = 1;

	return _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_ARRAY_INDEX, parent, srcs, 3);
}

static int _scf_op_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (0 == nb_nodes)
		return 0;

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);

	if (b->node._3ac_done)
		return 0;

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
				scf_loge("\n");
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_3ac_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			return -1;
		}

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
			scf_loge("\n");
			ast->current_block = prev_block;
			return -1;
		}

		// for goto
		if (SCF_LABEL == node->type) {
			scf_label_t* l = node->label;

			scf_list_t*     tail = scf_list_tail(d->_3ac_list_head);
			scf_3ac_code_t*	end  = scf_list_data(tail, scf_3ac_code_t, list);

			if (scf_vector_add(d->branch_ops->_labels, end) < 0) {
				scf_loge("\n");
				return -1;
			}
			end->label = l;

			int j;
			for (j = 0; j < d->branch_ops->_gotos->size; j++) {

				scf_3ac_code_t* c = d->branch_ops->_gotos->data[j];
				if (!c)
					continue;

				assert(l->w);
				assert(c->label->w);

				scf_logi("l: %s, c->label: %s\n", l->w->text->data, c->label->w->text->data);

				if (!strcmp(l->w->text->data, c->label->w->text->data)) {
					scf_logi("j: %d\n", j);
					scf_3ac_operand_t* dst = c->dsts->data[0];
					dst->code = end;
				}
			}
		}

		i++;
	}

	ast->current_block = prev_block;
	scf_logi("b: %p ok\n\n", b);
	return 0;
}

static int _scf_op_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;
	scf_function_t*     f = (scf_function_t*) ast->current_block;

	while (f && SCF_FUNCTION  != f->node.type)
		f = (scf_function_t*) f->node.parent;

	if (!f) {
		scf_loge("\n");
		return -1;
	}

	if (nb_nodes > f->rets->size) {
		scf_loge("\n");
		return -1;
	}

	scf_node_t* parent;
	scf_node_t* srcs[4];
	scf_expr_t* e;

	int i;
	for (i = 0; i < nb_nodes && i < 4; i++) {
		e  = nodes[i];

		if (_scf_expr_calculate_internal(ast, e->nodes[0], d) < 0) {
			scf_loge("\n");
			return -1;
		}

		srcs[i] = e->nodes[0];
	}

	if (i > 0) {
		if (_scf_3ac_code_srcN(d->_3ac_list_head, SCF_OP_RETURN, srcs, i) < 0)
			return -1;
	}

	scf_3ac_code_t* end = scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);

	scf_list_add_tail(d->_3ac_list_head, &end->list);

	return scf_vector_add(d->branch_ops->_ends, end);
}

static int _scf_op_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_node_t* n = (scf_node_t*)ast->current_block;

	while (n && (SCF_OP_WHILE != n->type && SCF_OP_FOR != n->type)) {
		n = n->parent;
	}

	if (!n) {
		scf_loge("\n");
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);

	scf_node_t* parent = n->parent;
	assert(parent);

	scf_3ac_code_t* branch = scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);

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
		scf_loge("\n");
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);

	scf_3ac_code_t* branch = scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);

	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	scf_vector_add(d->branch_ops->_continues, branch);
	return 0;
}

static int _scf_op_label(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
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

	scf_3ac_operand_t* dst;
	scf_3ac_code_t*    branch = scf_branch_ops_code(SCF_OP_GOTO, l, NULL);
	scf_list_add_tail(d->_3ac_list_head, &branch->list);

	int i;
	for (i = 0; i < d->branch_ops->_labels->size; i++) {
		scf_3ac_code_t* c = d->branch_ops->_labels->data[i];

		if (!strcmp(l->w->text->data, c->label->w->text->data)) {
			dst = branch->dsts->data[0];
			dst->code = c;
			break;
		}
	}

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

	scf_3ac_operand_t* dst;
	scf_3ac_code_t*    je   = scf_branch_ops_code(SCF_OP_3AC_JZ,  NULL, parent);
	scf_3ac_code_t*    jmp  = scf_branch_ops_code(SCF_OP_GOTO,    NULL, parent);
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
	dst = je->dsts->data[0];
	dst->code = jmp;

	// add 'jmp' to code list & error branches, it will be re-filled when this function end
	scf_list_add_tail(d->_3ac_list_head, &jmp->list);
	scf_vector_add(d->branch_ops->_errors, jmp);
	scf_vector_add(d->branch_ops->_errors, je); // 'je' will re-fill too.

	return 0;
}

static int _scf_op_cond(scf_ast_t* ast, scf_expr_t* e, scf_handler_data_t* d)
{
	if (_scf_expr_calculate_internal(ast, e->nodes[0], d) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* result = e->nodes[0]->result;
	assert(result);

	int is_default =  0;
	int jmp_op     = -1;

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
		case SCF_OP_LOGIC_NOT:
			jmp_op = SCF_OP_3AC_JNZ;
			break;

		default:
			if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, e->nodes[0]) < 0) {
				scf_loge("\n");
				return -1;
			}
			jmp_op     = SCF_OP_3AC_JZ;
			is_default = 1;
			break;
	};

	if (!is_default) {

		scf_list_t*     l    = scf_list_tail(d->_3ac_list_head);
		scf_3ac_code_t* c    = scf_list_data(l, scf_3ac_code_t, list);
		scf_vector_t*   dsts = c->dsts;

		if (SCF_OP_LOGIC_NOT == e->nodes[0]->type)
			c->op  = scf_3ac_find_operator(SCF_OP_3AC_TEQ);
		else
			c->op  = scf_3ac_find_operator(SCF_OP_3AC_CMP);
		c->dsts = NULL;

		scf_vector_clear(dsts, ( void (*)(void*) ) scf_3ac_operand_free);
		scf_vector_free (dsts);
		dsts = NULL;
	}

	return jmp_op;
}

static int _scf_op_node(scf_ast_t* ast, scf_node_t* node, scf_handler_data_t* d)
{
	scf_operator_t* op = node->op;

	if (!op) {
		op = scf_find_base_operator_by_type(node->type);
		if (!op) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_operator_handler_t*	h = scf_find_3ac_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		scf_loge("\n");
		return -1;
	}

	if (h->func(ast, node->nodes, node->nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}
	return 0;
}

static int _scf_op_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (2 != nb_nodes && 3 != nb_nodes) {
		scf_loge("\n");
		return -1;
	}

	scf_handler_data_t* d = data;

	scf_expr_t* e      = nodes[0];
	scf_node_t* parent = e->parent;

	int jmp_op = _scf_op_cond(ast, e, d);
	if (jmp_op < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_operand_t* dst;
	scf_3ac_code_t*    jmp_else  = scf_branch_ops_code(jmp_op, NULL, NULL);
	scf_3ac_code_t*    jmp_endif = NULL;
	scf_list_t*        l;

	scf_list_add_tail(d->_3ac_list_head, &jmp_else->list);

	int i;
	for (i = 1; i < nb_nodes; i++) {
		scf_node_t* node = nodes[i];

		if (_scf_op_node(ast, node, d) < 0) {
			scf_loge("\n");
			return -1;
		}

		if (1 == i) {
			if (3 == nb_nodes) {
				jmp_endif = scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);
				scf_list_add_tail(d->_3ac_list_head, &jmp_endif->list);
			}

			l   = scf_list_tail(d->_3ac_list_head);
			dst = jmp_else->dsts->data[0];
			dst->code = scf_list_data(l, scf_3ac_code_t, list);
		}
	}

	int ret = scf_vector_add(d->branch_ops->_breaks, jmp_else);
	if (ret < 0)
		return ret;

	if (jmp_endif) {
		l   = scf_list_tail(d->_3ac_list_head);
		dst = jmp_endif->dsts->data[0];
		dst->code = scf_list_data(l, scf_3ac_code_t, list);

		ret = scf_vector_add(d->branch_ops->_breaks, jmp_endif);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int _scf_op_end_loop(scf_list_t* start_prev, scf_3ac_code_t* jmp_end, scf_branch_ops_t* up_branch_ops, scf_handler_data_t* d)
{
	// change 'while (cond) {} ' to
	// 'if (cond) {
	//    do {
    //    } while (cond);
    //  }'
	// for optimizer

	// copy cond expr
	scf_list_t* l;
	scf_list_t* l2;
	scf_list_t* cond_prev = scf_list_tail(d->_3ac_list_head);

	for (l = scf_list_next(start_prev); l != &jmp_end->list; l = scf_list_next(l)) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_t* c2 = scf_3ac_code_clone(c);
		if (!c2)
			return -ENOMEM;

		scf_list_add_tail(d->_3ac_list_head, &c2->list);
	}

	for (l = scf_list_next(cond_prev); l != scf_list_sentinel(d->_3ac_list_head); l = scf_list_next(l)) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		if (!scf_type_is_jmp(c->op->type))
			continue;

		for (l2 = scf_list_next(cond_prev); l2 != scf_list_sentinel(d->_3ac_list_head); l2 = scf_list_next(l2)) {

			scf_3ac_code_t*    c2  = scf_list_data(l2, scf_3ac_code_t, list);
			scf_3ac_operand_t* dst = c->dsts->data[0];

			if (dst->code == c2->origin) {
				dst->code =  c2;
				break;
			}
		}
		assert(l2 != scf_list_sentinel(d->_3ac_list_head));

		if (scf_vector_add(d->branch_ops->_breaks, c) < 0)
			return -1;
	}

	int jmp_op = -1;
	switch (jmp_end->op->type) {

		case SCF_OP_3AC_JNZ:
			jmp_op = SCF_OP_3AC_JZ;
			break;
		case SCF_OP_3AC_JZ:
			jmp_op = SCF_OP_3AC_JNZ;
			break;
		case SCF_OP_3AC_JLE:
			jmp_op = SCF_OP_3AC_JGT;
			break;
		case SCF_OP_3AC_JLT:
			jmp_op = SCF_OP_3AC_JGE;
			break;
		case SCF_OP_3AC_JGE:
			jmp_op = SCF_OP_3AC_JLT;
			break;
		case SCF_OP_3AC_JGT:
			jmp_op = SCF_OP_3AC_JLE;
			break;

		default:
			jmp_op = -1;
			break;
	};

	// add loop when true
	scf_3ac_code_t*	loop = scf_branch_ops_code(jmp_op, NULL, NULL);
	scf_list_add_tail(d->_3ac_list_head, &loop->list);

	// should get the real start here,
	scf_3ac_code_t*	   start = scf_list_data(scf_list_next(&jmp_end->list), scf_3ac_code_t, list);
	scf_3ac_operand_t* dst   = loop->dsts->data[0];
	dst->code = start;

	// set jmp destination for 'continue',
	// it's the 'real' dst & needs not to re-fill
	int i;
	for (i = 0; i < d->branch_ops->_continues->size; i++) {
		scf_3ac_code_t*	branch = d->branch_ops->_continues->data[i];
		assert(branch->dsts);

		scf_3ac_operand_t* dst = branch->dsts->data[0];
		assert(!dst->code);

		dst->code = start;
	}

	// get the end, it's NOT the 'real' dst, but the prev of the 'real'
	scf_3ac_code_t*	end_prev  = scf_list_data(scf_list_tail(d->_3ac_list_head), scf_3ac_code_t, list);

	for (i = 0; i < d->branch_ops->_breaks->size; i++) {
		scf_3ac_code_t*	branch = d->branch_ops->_breaks->data[i];
		assert(branch->dsts);

		scf_3ac_operand_t* dst = branch->dsts->data[0];

		if (!dst->code)
			dst->code = end_prev;
	}

	if (jmp_end) {
		scf_3ac_operand_t* dst = jmp_end->dsts->data[0];

		dst->code = end_prev;
	}

	if (up_branch_ops) {
		for (i = 0; i < d->branch_ops->_breaks->size; i++) {
			scf_3ac_code_t*	branch = d->branch_ops->_breaks->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_breaks, branch) < 0)
				return -1;
		}

		if (jmp_end) {
			if (scf_vector_add(up_branch_ops->_breaks, jmp_end) < 0)
				return -1;
		}

		for (i = 0; i < d->branch_ops->_gotos->size; i++) {
			scf_3ac_code_t*	branch = d->branch_ops->_gotos->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_gotos, branch) < 0)
				return -1;
		}

		for (i = 0; i < d->branch_ops->_errors->size; i++) {
			scf_3ac_code_t*	branch = d->branch_ops->_errors->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_errors, branch) < 0)
				return -1;
		}

		for (i = 0; i < d->branch_ops->_ends->size; i++) {
			scf_3ac_code_t*	branch = d->branch_ops->_ends->data[i];
			assert(branch);

			if (scf_vector_add(up_branch_ops->_ends, branch) < 0)
				return -1;
		}
	}

	return 0;
}

static int _scf_op_while(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	// we don't know the real start of the while loop here,
	// we only know it's the next of 'start_prev'
	scf_list_t* start_prev = scf_list_tail(d->_3ac_list_head);

	int jmp_op = _scf_op_cond(ast, e, d);
	if (jmp_op < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_code_t* jmp_end = scf_branch_ops_code(jmp_op, NULL, NULL);
	scf_list_add_tail(d->_3ac_list_head, &jmp_end->list);

	scf_branch_ops_t* local_branch_ops = scf_branch_ops_alloc();
	scf_branch_ops_t* up_branch_ops    = d->branch_ops;
	d->branch_ops                      = local_branch_ops;

	// while body
	if (_scf_op_node(ast, nodes[1], d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (_scf_op_end_loop(start_prev, jmp_end, up_branch_ops, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	d->branch_ops    = up_branch_ops;
	scf_branch_ops_free(local_branch_ops);
	local_branch_ops = NULL;
	return 0;
}

static int _scf_op_for(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(4 == nb_nodes);

	scf_handler_data_t* d = data;

	if (nodes[0]) {
		if (_scf_op_node(ast, nodes[0], d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_list_t*     start_prev = scf_list_tail(d->_3ac_list_head);
	scf_3ac_code_t* jmp_end    = NULL;

	if (nodes[1]) {
		assert(SCF_OP_EXPR == nodes[1]->type);

		int jmp_op = _scf_op_cond(ast, nodes[1], d);
		if (jmp_op < 0) {
			scf_loge("\n");
			return -1;
		}

		jmp_end = scf_branch_ops_code(jmp_op, NULL, NULL);
		scf_list_add_tail(d->_3ac_list_head, &jmp_end->list);
	}

	scf_branch_ops_t* local_branch_ops = scf_branch_ops_alloc();
	scf_branch_ops_t* up_branch_ops    = d->branch_ops;
	d->branch_ops                      = local_branch_ops;

	if (nodes[3]) {
		if (_scf_op_node(ast, nodes[3], d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	if (nodes[2]) {
		if (_scf_op_node(ast, nodes[2], d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	if (_scf_op_end_loop(start_prev, jmp_end, up_branch_ops, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	d->branch_ops    = up_branch_ops;
	scf_branch_ops_free(local_branch_ops);
	local_branch_ops = NULL;
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
	scf_logi("f: %p, f->node->w: %s\n", f, f->node.w->text->data);

	scf_handler_data_t* d = data;

	// save & change the current block
	scf_block_t* prev_block = ast->current_block;
	ast->current_block = (scf_block_t*)f;

	scf_branch_ops_t* local_branch_ops = scf_branch_ops_alloc();
	scf_branch_ops_t* tmp_branch_ops   = d->branch_ops;
	d->branch_ops 					   = local_branch_ops;

	// use local_branch_ops, because branch code should NOT jmp over the function block
	if (_scf_op_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_END, NULL) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_3ac_code_t* err;
	scf_node_t*     node;
	scf_list_t*     tail;
	scf_list_t*     next;
	int i;
	int ret;

	// re-fill 'break'
	for (i = 0; i < local_branch_ops->_breaks->size; i++) {

		scf_3ac_code_t*    c   = local_branch_ops->_breaks->data[i];
		scf_3ac_operand_t* dst = c->dsts->data[0];

		if (dst->code) {
			next      = scf_list_next(&dst->code->list);
			dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("'break' has a bug!\n");
			return -1;
		}
	}

	// re-fill 'goto'
	for (i = 0; i < local_branch_ops->_gotos->size; i++) {

		scf_3ac_code_t*    c    = local_branch_ops->_gotos->data[i];
		scf_3ac_operand_t* dst  = c->dsts->data[0];

		if (dst->code) {
			next      = scf_list_next(&dst->code->list);
			dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("all 'goto' should get its label in this function\n");
			return -1;
		}
	}

	// re-fill 'end'
	for (i = 0; i < local_branch_ops->_ends->size; i++) {

		scf_3ac_code_t*    c    = local_branch_ops->_ends->data[i];
		scf_list_t*        l    = scf_list_tail(d->_3ac_list_head);
		scf_3ac_code_t*    end  = scf_list_data(l, scf_3ac_code_t, list);
		scf_3ac_operand_t* dst  = c->dsts->data[0];

		assert(!dst->code);

		if (&c->list == scf_list_prev(l))
			c->op = scf_3ac_find_operator(SCF_OP_3AC_NOP);
		else
			dst->code = end;
	}

	if (0 == local_branch_ops->_errors->size)
		goto end;

	// do all the 'error'.
	for (i = local_branch_ops->_errors->size - 1; i >= 0; i--) {

		err = local_branch_ops->_errors->data[i];
		assert(err);

		scf_3ac_operand_t* dst = err->dsts->data[0];

		if (!dst->code) {
			assert(err->error);

			node = err->error;
			err->error = NULL;

			tail = scf_list_tail(d->_3ac_list_head);
			dst->code = scf_list_data(tail, scf_3ac_code_t, list);

			ret = _scf_do_error(ast, node, d);
			SCF_CHECK_ERROR(ret < 0, ret, "do error failed\n");
		} else {
			// it is the 'je' when error don't happen, do nothing.
		}
	}

	// pop & return error code
	_scf_3ac_code_dst(d->_3ac_list_head, SCF_OP_3AC_POP, NULL);

	if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_END, NULL) < 0) {
		scf_loge("\n");
		return -1;
	}

	// re-fill 'error'
	for (i = 0; i < local_branch_ops->_errors->size; i++) {

		scf_3ac_code_t*    c   = local_branch_ops->_errors->data[i];
		scf_3ac_operand_t* dst = c->dsts->data[0];

		if (dst->code) {
			next      = scf_list_next(&dst->code->list);
			dst->code = scf_list_data(next, scf_3ac_code_t, list);
		} else {
			scf_loge("'error' has a bug!\n");
			return -1;
		}
	}

end:
	scf_branch_ops_free(local_branch_ops);
	local_branch_ops = NULL;
	d->branch_ops    = tmp_branch_ops;

	ast->current_block = prev_block;
	return 0;
}

int scf_function_to_3ac(scf_ast_t* ast, scf_function_t* f, scf_list_t* _3ac_list_head)
{
	scf_logi("f: %p\n", f);

	scf_handler_data_t d = {0};
	d._3ac_list_head  	 = _3ac_list_head;

	int ret = __scf_op_call(ast, f, &d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_logi("f: %p ok\n\n", f);
	return 0;
}

static int _scf_op_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_logw("\n\n");

	assert(nb_nodes > 0);

	scf_handler_data_t* d      = data;
	scf_variable_t*     v      = NULL;
	scf_function_t*     f      = NULL;
	scf_vector_t*       argv   = NULL;
	scf_node_t*         parent = nodes[0]->parent;

	int i;
	int ret = _scf_expr_calculate_internal(ast, nodes[0], d);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	v = _scf_operand_get(nodes[0]);
	f = v->func_ptr;

	if (f->vargs_flag) {
		if (f->argv->size > nb_nodes - 1)
			return -1;
	} else if (f->argv->size != nb_nodes - 1)
		return -1;

	argv = scf_vector_alloc();
	if (!argv) {
		scf_loge("\n");
		return -ENOMEM;
	}

	ret = scf_vector_add(argv, nodes[0]);
	if (ret < 0) {
		scf_vector_free(argv);
		return ret;
	}

	for (i = 1; i < nb_nodes; i++) {
		scf_node_t*     arg   = nodes[i];
		scf_node_t*     child = NULL;

		while (SCF_OP_EXPR == arg->type)
			arg = arg->nodes[0];

		if (scf_type_is_assign(arg->type)) {

			assert(2 == arg->nb_nodes);
			child     = arg->nodes[0];

			child->_3ac_done = 0;

			ret = _scf_expr_calculate_internal(ast, child, d);
			if (ret < 0) {
				scf_vector_free(argv);
				return ret;
			}

			arg = child;
		}

		ret = scf_vector_add(argv, arg);
		if (ret < 0) {
			scf_vector_free(argv);
			return ret;
		}

		v = _scf_operand_get(arg);
		v->local_flag = 1;
	}


	if (parent->result_nodes) {

		scf_node_t* node;

		for (i = 0; i < parent->result_nodes->size; i++) {
			node      = parent->result_nodes->data[i];

			v = _scf_operand_get(node);
			v->local_flag = 1;
		}

		ret = _scf_3ac_code_NN(d->_3ac_list_head, SCF_OP_CALL,
				(scf_node_t**)parent->result_nodes->data, parent->result_nodes->size,
				(scf_node_t**)argv->data, argv->size);

	} else {
		v = _scf_operand_get(parent);
		if (v)
			v->local_flag = 1;

		ret = _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_CALL, parent, (scf_node_t**)argv->data, argv->size);
	}
	scf_logw("f: %p, ok\n\n", f);

	scf_vector_free(argv);
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

#define SCF_OP_UNARY(name, op_type) \
static int _scf_op_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(1 == nb_nodes); \
	scf_handler_data_t* d = data; \
	scf_node_t* parent    = nodes[0]->parent; \
	return _scf_3ac_code_2(d->_3ac_list_head, op_type, parent, nodes[0]); \
}

SCF_OP_UNARY(neg,         SCF_OP_NEG)
SCF_OP_UNARY(positive,    SCF_OP_POSITIVE)
SCF_OP_UNARY(dereference, SCF_OP_DEREFERENCE)
SCF_OP_UNARY(logic_not,   SCF_OP_LOGIC_NOT)
SCF_OP_UNARY(bit_not,     SCF_OP_BIT_NOT)

static int _scf_op_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent    = nodes[0]->parent;
	scf_node_t* child     = nodes[0];

	if (scf_type_is_var(child->type))
		return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ADDRESS_OF, parent, child);

	if (SCF_OP_ARRAY_INDEX == child->type) {
		assert(2 == child->nb_nodes);

		scf_node_t* n_scale = NULL;
		scf_node_t* srcs[3];

		int i;
		for (i = 0; i < 2; i++) {
			if (_scf_expr_calculate_internal(ast, child->nodes[i], d) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		int ret = _scf_op_array_scale(ast, child, &n_scale);
		if (ret < 0)
			return ret;

		srcs[0] = child->nodes[0];
		srcs[1] = child->nodes[1];
		srcs[2] = n_scale;

		return _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX, parent, srcs, 3);
	}

	if (SCF_OP_POINTER == child->type) {
		assert(2 == child->nb_nodes);

		scf_node_t* srcs[2];

		int i;
		for (i = 0; i < 2; i++) {
			if (_scf_expr_calculate_internal(ast, child->nodes[i], d) < 0) {
				scf_loge("\n");
				return -1;
			}
		}

		srcs[0] = child->nodes[0];
		srcs[1] = child->nodes[1];

		return _scf_3ac_code_N(d->_3ac_list_head, SCF_OP_3AC_ADDRESS_OF_POINTER, parent, srcs, 2);
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_TYPE_CAST, parent, nodes[1]);
}

#define SCF_OP_BINARY(name, op_type) \
static int _scf_op_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(2 == nb_nodes); \
	scf_handler_data_t* d = data; \
	scf_node_t* parent = nodes[0]->parent; \
	return _scf_3ac_code_3(d->_3ac_list_head, op_type, parent, nodes[0], nodes[1]); \
}

SCF_OP_BINARY(add,     SCF_OP_ADD)
SCF_OP_BINARY(sub,     SCF_OP_SUB)
SCF_OP_BINARY(mul,     SCF_OP_MUL)
SCF_OP_BINARY(div,     SCF_OP_DIV)
SCF_OP_BINARY(mod,     SCF_OP_MOD)
SCF_OP_BINARY(shl,     SCF_OP_SHL)
SCF_OP_BINARY(shr,     SCF_OP_SHR)
SCF_OP_BINARY(bit_and, SCF_OP_BIT_AND)
SCF_OP_BINARY(bit_or,  SCF_OP_BIT_OR)

static int _scf_op_left_value_array_index(scf_ast_t* ast, int type, scf_node_t* left, scf_node_t* right, scf_handler_data_t* d)
{
	assert(2 == left->nb_nodes);

	scf_node_t* n_index = NULL;
	scf_node_t* n_scale = NULL;
	scf_node_t* srcs[4];

	int i;
	for (i = 0; i < 2; i++) {
		if (_scf_expr_calculate_internal(ast, left->nodes[i], d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	n_index = left->nodes[1];
	while (SCF_OP_EXPR == n_index->type)
		n_index = n_index->nodes[0];

	int ret = _scf_op_array_scale(ast, left, &n_scale);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	srcs[0]   = left->nodes[0];
	srcs[1]   = n_index;
	srcs[i++] = n_scale;
	if (right)
		srcs[i++] = right;

	if (_scf_3ac_code_N(d->_3ac_list_head, type, NULL, srcs, i) < 0) {
		scf_loge("\n");
		return -1;
	}
	return 0;
}

static int _scf_op_left_value(scf_ast_t* ast, int type, scf_node_t* left, scf_node_t* right, scf_handler_data_t* d)
{
	assert(1 == left->nb_nodes || 2 == left->nb_nodes);

	scf_node_t* srcs[3];

	int i;
	for (i = 0; i < left->nb_nodes; i++) {
		if (_scf_expr_calculate_internal(ast, left->nodes[i], d) < 0) {
			scf_loge("\n");
			return -1;
		}

		srcs[i] = left->nodes[i];
	}

	if (right)
		srcs[i++] = right;

	assert(i <= 3);

	if (_scf_3ac_code_N(d->_3ac_list_head, type, NULL, srcs, i) < 0) {
		scf_loge("\n");
		return -1;
	}
	return 0;
}

static int _scf_op_right_value(scf_ast_t* ast, scf_node_t* right, scf_handler_data_t* d)
{
	if (_scf_expr_calculate_internal(ast, right, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (scf_type_is_assign(right->type)) {
		right = right->nodes[0];

		while (SCF_OP_EXPR == right->type)
			right = right->nodes[0];

		assert(!scf_type_is_assign(right->type));

		right->_3ac_done = 0;

		if (_scf_expr_calculate_internal(ast, right, d) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	return 0;
}

static int _scf_op_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);
	scf_handler_data_t* d = data;

	scf_node_t*     parent = nodes[0]->parent;
	scf_node_t*     node0  = nodes[0];
	scf_node_t*     node1  = nodes[1];
	scf_variable_t* v0     = _scf_operand_get(node0);

	if ( _scf_op_right_value(ast, node1, d) < 0)
		return -1;

	while (SCF_OP_EXPR == node0->type)
		node0 = node0->nodes[0];

	switch (node0->type) {
		case SCF_OP_DEREFERENCE:
			return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_DEREFERENCE, node0, node1, d);
			break;
		case SCF_OP_ARRAY_INDEX:
			return _scf_op_left_value_array_index(ast, SCF_OP_3AC_ASSIGN_ARRAY_INDEX, node0, node1, d);
			break;
		case SCF_OP_POINTER:
			return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_POINTER, node0, node1, d);
			break;
		default:
			break;
	};

	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_ASSIGN, node0, node1);
}

#define SCF_OP_BINARY_ASSIGN(name, op) \
static int _scf_op_##name##_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(2 == nb_nodes); \
	scf_handler_data_t* d = data; \
	\
	scf_node_t*     parent = nodes[0]->parent; \
	scf_node_t*     node0  = nodes[0]; \
	scf_node_t*     node1  = nodes[1]; \
	scf_variable_t* v1     = _scf_operand_get(nodes[1]); \
	\
	if ( _scf_op_right_value(ast, node1, d) < 0) \
		return -1; \
	\
	while (SCF_OP_EXPR == node0->type) \
		node0 = node0->nodes[0]; \
	\
	int is_float = scf_type_is_float(v1->type) && 0 == v1->nb_pointers; \
	if (is_float) { \
		if (_scf_expr_calculate_internal(ast, node0, d) < 0) { \
			scf_loge("\n"); \
			return -1; \
		} \
		\
		if ( _scf_3ac_code_2(d->_3ac_list_head, parent->type, node0, node1) < 0) { \
			scf_loge("\n"); \
			return -1; \
		} \
		\
		switch (node0->type) { \
			case SCF_OP_DEREFERENCE: \
				return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_DEREFERENCE, node0, node0, d); \
				break; \
			case SCF_OP_ARRAY_INDEX: \
				return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_ARRAY_INDEX, node0, node0, d); \
				break; \
			case SCF_OP_POINTER: \
				return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_POINTER, node0, node0, d); \
				break; \
			default: \
				break; \
		}; \
		\
		return 0; \
	} \
	\
	switch (node0->type) { \
		case SCF_OP_DEREFERENCE: \
			return _scf_op_left_value(ast, SCF_OP_3AC_##op##_ASSIGN_DEREFERENCE, node0, node1, d); \
			break; \
		case SCF_OP_ARRAY_INDEX: \
			return _scf_op_left_value_array_index(ast, SCF_OP_3AC_##op##_ASSIGN_ARRAY_INDEX, node0, node1, d); \
			break; \
		case SCF_OP_POINTER: \
			return _scf_op_left_value(ast, SCF_OP_3AC_##op##_ASSIGN_POINTER, node0, node1, d); \
			break; \
		default: \
			break; \
	}; \
	\
	return _scf_3ac_code_2(d->_3ac_list_head, SCF_OP_##op##_ASSIGN, node0, node1); \
}

SCF_OP_BINARY_ASSIGN(add, ADD)
SCF_OP_BINARY_ASSIGN(sub, SUB)
SCF_OP_BINARY_ASSIGN(and, AND)
SCF_OP_BINARY_ASSIGN(or,  OR)

#define SCF_OP_BINARY_ASSIGN2(name, op) \
static int _scf_op_##name##_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(2 == nb_nodes); \
	scf_handler_data_t* d = data; \
	\
	scf_node_t*     parent = nodes[0]->parent; \
	scf_node_t*     node0  = nodes[0]; \
	scf_node_t*     node1  = nodes[1]; \
	scf_variable_t* v1     = _scf_operand_get(nodes[1]); \
	\
	if ( _scf_op_right_value(ast, node1, d) < 0) \
		return -1; \
	\
	while (SCF_OP_EXPR == node0->type) \
		node0 = node0->nodes[0]; \
	\
	if (_scf_expr_calculate_internal(ast, node0, d) < 0) { \
		scf_loge("\n"); \
		return -1; \
	} \
	\
	if ( _scf_3ac_code_2(d->_3ac_list_head, parent->type, node0, node1) < 0) { \
		scf_loge("\n"); \
		return -1; \
	} \
	\
	switch (node0->type) { \
		case SCF_OP_DEREFERENCE: \
			return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_DEREFERENCE, node0, node0, d); \
			break; \
		case SCF_OP_ARRAY_INDEX: \
			return _scf_op_left_value_array_index(ast, SCF_OP_3AC_ASSIGN_ARRAY_INDEX, node0, node0, d); \
			break; \
		case SCF_OP_POINTER: \
			return _scf_op_left_value(ast, SCF_OP_3AC_ASSIGN_POINTER, node0, node0, d); \
			break; \
		default: \
			break; \
	}; \
	\
	return 0; \
}
SCF_OP_BINARY_ASSIGN2(shl, SHL)
SCF_OP_BINARY_ASSIGN2(shr, SHR)
SCF_OP_BINARY_ASSIGN2(mul, MUL)
SCF_OP_BINARY_ASSIGN2(div, DIV)
SCF_OP_BINARY_ASSIGN2(mod, MOD)

#define SCF_OP_UNARY_ASSIGN(name, op) \
static int __scf_op_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(1 == nb_nodes); \
	scf_handler_data_t* d = data; \
	\
	scf_node_t*     node0 = nodes[0]; \
	\
	while (SCF_OP_EXPR == node0->type) \
		node0 = node0->nodes[0]; \
	\
	switch (node0->type) { \
		case SCF_OP_DEREFERENCE: \
			return _scf_op_left_value(ast, SCF_OP_3AC_##op##_DEREFERENCE, node0, NULL, d); \
			break; \
		case SCF_OP_ARRAY_INDEX: \
			return _scf_op_left_value_array_index(ast, SCF_OP_3AC_##op##_ARRAY_INDEX, node0, NULL, d); \
			break; \
		case SCF_OP_POINTER: \
			return _scf_op_left_value(ast, SCF_OP_3AC_##op##_POINTER, node0, NULL, d); \
			break; \
		default: \
			break; \
	}; \
	\
	return _scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_##op, node0); \
}
SCF_OP_UNARY_ASSIGN(inc, INC)
SCF_OP_UNARY_ASSIGN(dec, DEC)

#define SCF_OP_UNARY_ASSIGN2(name0, name1, op_type, post_flag) \
static int _scf_op_##name0(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	assert(1 == nb_nodes); \
	scf_handler_data_t* d  = data; \
	\
	scf_node_t*     node0  = nodes[0]; \
	scf_node_t*     parent = nodes[0]->parent; \
	scf_variable_t* v      = _scf_operand_get(parent); \
	\
	int ret = __scf_op_##name1(ast, nodes, nb_nodes, data); \
	if (ret < 0) { \
		scf_loge("\n"); \
		return -1; \
	} \
	scf_list_t*     l  = scf_list_tail(d->_3ac_list_head); \
	scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list); \
	scf_3ac_code_t* c2 = scf_3ac_code_clone(c); \
	if (!c2) \
		return -ENOMEM; \
	\
	assert(!c2->dsts); \
	c2->dsts = scf_vector_alloc(); \
	if (!c2->dsts) { \
		scf_3ac_code_free(c2); \
		return -ENOMEM; \
	} \
	\
	scf_3ac_operand_t* dst = scf_3ac_operand_alloc(); \
	if (!dst) { \
		scf_3ac_code_free(c2); \
		return -ENOMEM; \
	} \
	if (scf_vector_add(c2->dsts, dst) < 0) { \
		scf_3ac_code_free(c2); \
		return -ENOMEM; \
	} \
	dst->node = parent; \
	v->local_flag = 1; \
	\
	switch (c2->op->type) { \
		case SCF_OP_3AC_##op_type##_DEREFERENCE: \
			c2->op = scf_3ac_find_operator(SCF_OP_3AC_ASSIGN_DEREFERENCE); \
			break; \
		case SCF_OP_3AC_##op_type##_ARRAY_INDEX: \
			c2->op = scf_3ac_find_operator(SCF_OP_3AC_ASSIGN_ARRAY_INDEX); \
		case SCF_OP_3AC_##op_type##_POINTER: \
			c2->op = scf_3ac_find_operator(SCF_OP_3AC_ASSIGN_POINTER); \
		default: \
			c2->op = scf_3ac_find_operator(SCF_OP_ASSIGN); \
			break; \
	}; \
	if (post_flag) \
		scf_list_add_tail(&c->list, &c2->list); \
	else \
		scf_list_add_front(&c->list, &c2->list); \
	return 0; \
}
SCF_OP_UNARY_ASSIGN2(inc,      inc, INC, 0)
SCF_OP_UNARY_ASSIGN2(dec,      dec, DEC, 0)
SCF_OP_UNARY_ASSIGN2(inc_post, inc, INC, 1)
SCF_OP_UNARY_ASSIGN2(dec_post, dec, DEC, 1)

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

static int _scf_op_logic_and_jmp(scf_ast_t* ast, scf_node_t* node, scf_handler_data_t* d)
{
	scf_node_t* parent = node->parent;

	if (_scf_expr_calculate_internal(ast, node, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	while (SCF_OP_EXPR == node->type)
		node = node->nodes[0];

	int is_default = 0;
	int jmp_op;
	int set_op;
	switch (node->type) {
		case SCF_OP_EQ:
			set_op = SCF_OP_3AC_SETZ;
			jmp_op = SCF_OP_3AC_JNZ;
			break;
		case SCF_OP_NE:
			set_op = SCF_OP_3AC_SETNZ;
			jmp_op = SCF_OP_3AC_JZ;
			break;
		case SCF_OP_GT:
			set_op = SCF_OP_3AC_SETGT;
			jmp_op = SCF_OP_3AC_JLE;
			break;
		case SCF_OP_GE:
			set_op = SCF_OP_3AC_SETGE;
			jmp_op = SCF_OP_3AC_JLT;
			break;
		case SCF_OP_LT:
			set_op = SCF_OP_3AC_SETLT;
			jmp_op = SCF_OP_3AC_JGE;
			break;
		case SCF_OP_LE:
			set_op = SCF_OP_3AC_SETLE;
			jmp_op = SCF_OP_3AC_JGT;
			break;

		default:
			if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, node) < 0) {
				scf_loge("\n");
				return -1;
			}

			if (_scf_3ac_code_dst(d->_3ac_list_head, SCF_OP_3AC_SETNZ, parent) < 0) {
				scf_loge("\n");
				return -1;
			}

			jmp_op     = SCF_OP_3AC_JZ;
			is_default = 1;
			break;
	};

	if (!is_default) {
		scf_list_t*     l    = scf_list_tail(d->_3ac_list_head);
		scf_3ac_code_t* c    = scf_list_data(l, scf_3ac_code_t, list);
		scf_vector_t*   dsts = c->dsts;

		c->op   = scf_3ac_find_operator(SCF_OP_3AC_CMP);
		c->dsts = NULL;

		scf_vector_clear(dsts, ( void (*)(void*) ) scf_3ac_operand_free);
		scf_vector_free (dsts);
		dsts = NULL;

		if (_scf_3ac_code_dst(d->_3ac_list_head, set_op, parent) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_variable_t* v = _scf_operand_get(parent);
	v->local_flag = 1;
	return jmp_op;
}

static int _scf_op_logic_or_jmp(scf_ast_t* ast, scf_node_t* node, scf_handler_data_t* d)
{
	scf_node_t* parent = node->parent;

	if (_scf_expr_calculate_internal(ast, node, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	while (SCF_OP_EXPR == node->type)
		node = node->nodes[0];

	int is_default = 0;
	int jmp_op;
	int set_op;
	switch (node->type) {
		case SCF_OP_EQ:
			set_op = SCF_OP_3AC_SETZ;
			jmp_op = SCF_OP_3AC_JZ;
			break;
		case SCF_OP_NE:
			set_op = SCF_OP_3AC_SETNZ;
			jmp_op = SCF_OP_3AC_JNZ;
			break;
		case SCF_OP_GT:
			set_op = SCF_OP_3AC_SETGT;
			jmp_op = SCF_OP_3AC_JGT;
			break;
		case SCF_OP_GE:
			set_op = SCF_OP_3AC_SETGE;
			jmp_op = SCF_OP_3AC_JGE;
			break;
		case SCF_OP_LT:
			set_op = SCF_OP_3AC_SETLT;
			jmp_op = SCF_OP_3AC_JLT;
			break;
		case SCF_OP_LE:
			set_op = SCF_OP_3AC_SETLE;
			jmp_op = SCF_OP_3AC_JLE;
			break;

		default:
			if (_scf_3ac_code_1(d->_3ac_list_head, SCF_OP_3AC_TEQ, node) < 0) {
				scf_loge("\n");
				return -1;
			}

			if (_scf_3ac_code_dst(d->_3ac_list_head, SCF_OP_3AC_SETNZ, parent) < 0) {
				scf_loge("\n");
				return -1;
			}

			jmp_op     = SCF_OP_3AC_JNZ;
			is_default = 1;
			break;
	};

	if (!is_default) {
		scf_list_t*     l    = scf_list_tail(d->_3ac_list_head);
		scf_3ac_code_t* c    = scf_list_data(l, scf_3ac_code_t, list);
		scf_vector_t*   dsts = c->dsts;

		c->op   = scf_3ac_find_operator(SCF_OP_3AC_CMP);
		c->dsts = NULL;

		scf_vector_clear(dsts, ( void(*)(void*) ) scf_3ac_operand_free);
		scf_vector_free (dsts);
		dsts = NULL;

		if (_scf_3ac_code_dst(d->_3ac_list_head, set_op, parent) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_variable_t* v = _scf_operand_get(parent);
	v->local_flag = 1;
	return jmp_op;
}

#define SCF_OP_LOGIC(name) \
static int _scf_op_logic_##name(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data) \
{ \
	scf_handler_data_t* d = data; \
	scf_node_t* parent    = nodes[0]->parent; \
	int jmp_op = _scf_op_logic_##name##_jmp(ast, nodes[0], d); \
	if (jmp_op < 0) \
		return -1; \
	\
	scf_3ac_code_t* jmp = scf_branch_ops_code(jmp_op, NULL, NULL); \
	if (!jmp) \
		return -ENOMEM; \
	\
	scf_list_add_tail(d->_3ac_list_head, &jmp->list); \
	if (_scf_op_logic_##name##_jmp(ast, nodes[1], d) < 0) \
		return -1; \
	\
	jmp->dsts = scf_vector_alloc(); \
	if (!jmp->dsts) \
		return -ENOMEM; \
	scf_3ac_operand_t* dst = scf_3ac_operand_alloc(); \
	if (!dst) \
		return -ENOMEM; \
	if (scf_vector_add(jmp->dsts, dst) < 0) \
		return -ENOMEM; \
	\
	scf_list_t* l  = scf_list_tail(d->_3ac_list_head); \
	dst->code = scf_list_data(l, scf_3ac_code_t, list); \
	\
	int ret = scf_vector_add(d->branch_ops->_breaks, jmp); \
	if (ret < 0) \
		return ret; \
	return 0; \
}
SCF_OP_LOGIC(and)
SCF_OP_LOGIC(or)

scf_operator_handler_t _3ac_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,           -1,   -1, -1, _scf_op_expr},
	{{NULL, NULL}, SCF_OP_CALL,           -1,   -1, -1, _scf_op_call},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,    -1,   -1, -1, _scf_op_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,        -1,   -1, -1, _scf_op_pointer},
	{{NULL, NULL}, SCF_OP_CREATE,         -1,   -1, -1, _scf_op_create},

	{{NULL, NULL}, SCF_OP_TYPE_CAST,      -1,   -1, -1, _scf_op_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,      -1,   -1, -1, _scf_op_logic_not},
	{{NULL, NULL}, SCF_OP_BIT_NOT,        -1,   -1, -1, _scf_op_bit_not},
	{{NULL, NULL}, SCF_OP_NEG,            -1,   -1, -1, _scf_op_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,       -1,   -1, -1, _scf_op_positive},

	{{NULL, NULL}, SCF_OP_INC,            -1,   -1, -1, _scf_op_inc},
	{{NULL, NULL}, SCF_OP_DEC,            -1,   -1, -1, _scf_op_dec},

	{{NULL, NULL}, SCF_OP_INC_POST,       -1,   -1, -1, _scf_op_inc_post},
	{{NULL, NULL}, SCF_OP_DEC_POST,       -1,   -1, -1, _scf_op_dec_post},

	{{NULL, NULL}, SCF_OP_DEREFERENCE,    -1,   -1, -1, _scf_op_dereference},
	{{NULL, NULL}, SCF_OP_ADDRESS_OF,     -1,   -1, -1, _scf_op_address_of},

	{{NULL, NULL}, SCF_OP_MUL,            -1,   -1, -1, _scf_op_mul},
	{{NULL, NULL}, SCF_OP_DIV,            -1,   -1, -1, _scf_op_div},
	{{NULL, NULL}, SCF_OP_MOD,            -1,   -1, -1, _scf_op_mod},

	{{NULL, NULL}, SCF_OP_ADD,            -1,   -1, -1, _scf_op_add},
	{{NULL, NULL}, SCF_OP_SUB,            -1,   -1, -1, _scf_op_sub},

	{{NULL, NULL}, SCF_OP_SHL,            -1,   -1, -1, _scf_op_shl},
	{{NULL, NULL}, SCF_OP_SHR,            -1,   -1, -1, _scf_op_shr},

	{{NULL, NULL}, SCF_OP_BIT_AND,        -1,   -1, -1, _scf_op_bit_and},
	{{NULL, NULL}, SCF_OP_BIT_OR,         -1,   -1, -1, _scf_op_bit_or},

	{{NULL, NULL}, SCF_OP_EQ,             -1,   -1, -1, _scf_op_eq},
	{{NULL, NULL}, SCF_OP_NE,             -1,   -1, -1, _scf_op_ne},
	{{NULL, NULL}, SCF_OP_GT,             -1,   -1, -1, _scf_op_gt},
	{{NULL, NULL}, SCF_OP_LT,             -1,   -1, -1, _scf_op_lt},
	{{NULL, NULL}, SCF_OP_GE,             -1,   -1, -1, _scf_op_ge},
	{{NULL, NULL}, SCF_OP_LE,             -1,   -1, -1, _scf_op_le},

	{{NULL, NULL}, SCF_OP_LOGIC_AND,      -1,   -1, -1, _scf_op_logic_and},
	{{NULL, NULL}, SCF_OP_LOGIC_OR,       -1,   -1, -1, _scf_op_logic_or},

	{{NULL, NULL}, SCF_OP_ASSIGN,		  -1,   -1, -1, _scf_op_assign},
	{{NULL, NULL}, SCF_OP_ADD_ASSIGN,     -1,   -1, -1, _scf_op_add_assign},
	{{NULL, NULL}, SCF_OP_SUB_ASSIGN,     -1,   -1, -1, _scf_op_sub_assign},
	{{NULL, NULL}, SCF_OP_MUL_ASSIGN,     -1,   -1, -1, _scf_op_mul_assign},
	{{NULL, NULL}, SCF_OP_DIV_ASSIGN,     -1,   -1, -1, _scf_op_div_assign},
	{{NULL, NULL}, SCF_OP_MOD_ASSIGN,     -1,   -1, -1, _scf_op_mod_assign},
	{{NULL, NULL}, SCF_OP_SHL_ASSIGN,     -1,   -1, -1, _scf_op_shl_assign},
	{{NULL, NULL}, SCF_OP_SHR_ASSIGN,     -1,   -1, -1, _scf_op_shr_assign},
	{{NULL, NULL}, SCF_OP_AND_ASSIGN,     -1,   -1, -1, _scf_op_and_assign},
	{{NULL, NULL}, SCF_OP_OR_ASSIGN,      -1,   -1, -1, _scf_op_or_assign},


	{{NULL, NULL}, SCF_OP_BLOCK,          -1,   -1, -1, _scf_op_block},
	{{NULL, NULL}, SCF_OP_RETURN,         -1,   -1, -1, _scf_op_return},
	{{NULL, NULL}, SCF_OP_BREAK,          -1,   -1, -1, _scf_op_break},
	{{NULL, NULL}, SCF_OP_CONTINUE,       -1,   -1, -1, _scf_op_continue},
	{{NULL, NULL}, SCF_OP_GOTO,           -1,   -1, -1, _scf_op_goto},
	{{NULL, NULL}, SCF_LABEL,             -1,   -1, -1, _scf_op_label},
	{{NULL, NULL}, SCF_OP_ERROR,          -1,   -1, -1, _scf_op_error},

	{{NULL, NULL}, SCF_OP_IF,             -1,   -1, -1, _scf_op_if},
	{{NULL, NULL}, SCF_OP_WHILE,          -1,   -1, -1, _scf_op_while},
	{{NULL, NULL}, SCF_OP_FOR,            -1,   -1, -1, _scf_op_for},
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

