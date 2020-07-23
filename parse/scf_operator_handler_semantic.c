#include"scf_ast.h"
#include"scf_operator_handler_semantic.h"
#include"scf_type_cast.h"

typedef struct {
	scf_variable_t**	pret;

} scf_handler_data_t;

static int _semantic_add_type_cast(scf_ast_t* ast, scf_node_t** pp, scf_variable_t* v_dst, scf_node_t* src)
{
	scf_node_t* parent = src->parent;

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_TYPE_CAST);
	if (!op)
		return -EINVAL;

	scf_type_t*     t = scf_ast_find_type_type(ast, v_dst->type);
	scf_variable_t* v = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 1, v_dst->nb_pointers, v_dst->func_ptr);
	if (!v)
		return -ENOMEM;

	scf_node_t* dst = scf_node_alloc(NULL, v->type, v);
	scf_variable_free(v);
	v = NULL;
	if (!dst)
		return -ENOMEM;

	scf_node_t* cast = scf_node_alloc(NULL, SCF_OP_TYPE_CAST, NULL);
	if (!cast) {
		scf_node_free(dst);
		return -ENOMEM;
	}

	int ret = scf_node_add_child(cast, dst);
	if (ret < 0) {
		scf_node_free(dst);
		scf_node_free(cast);
		return ret;
	}

	ret = scf_node_add_child(cast, src);
	if (ret < 0) {
		scf_node_free(cast); // dst is cast's child, will be recursive freed
		return ret;
	}

	cast->op     = op;
	cast->result = scf_variable_clone(dst->var);
	cast->parent = parent;
	*pp = cast;
	return 0;
}

static int _semantic_do_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	int ret = scf_find_updated_type(ast, v0, v1);
	if (ret < 0) {
		scf_loge("var type update failed, type: %d, %d\n", v0->type, v1->type);
		return -EINVAL;
	}

	scf_type_t*     t     = scf_ast_find_type_type(ast, ret);

	scf_variable_t* v_std = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 1, 0, NULL);
	if (!v_std)
		return -ENOMEM;

	if (t->type != v0->type) {
		ret = _semantic_add_type_cast(ast, &nodes[0], v_std, nodes[0]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			goto end;
		}
	}

	if (t->type != v1->type) {
		ret = _semantic_add_type_cast(ast, &nodes[1], v_std, nodes[1]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			goto end;
		}
	}

	ret = 0;
end:
	scf_variable_free(v_std);
	return ret;
}

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	if (!node) {
		return 0;
	}

	if (0 == node->nb_nodes) {
		scf_logd("node->type: %d\n", node->type);
		if (scf_type_is_var(node->type)) {
			scf_logd("node->var->w->text->data: %s\n", node->var->w->text->data);
		}
		assert(scf_type_is_var(node->type) || SCF_LABEL == node->type);
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

	if (node->result) {
		// free the result calculated before
		scf_variable_free(node->result);
		node->result = NULL;
	}

	if (node->w) {
		scf_logd("node: %p, node->w->text->data: %s\n", node, node->w->text->data);
	}

	scf_handler_data_t* d    = data;
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
		scf_operator_handler_t* h = scf_find_semantic_operator_handler(node->op->type, -1, -1, -1);
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
		scf_operator_handler_t* h = scf_find_semantic_operator_handler(node->op->type, -1, -1, -1);
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
	return 0;

_error:
	d->pret = pret;
	return -1;
}

static int _scf_expr_calculate(scf_ast_t* ast, scf_expr_t* e, scf_variable_t** pret)
{
	assert(e);
	assert(e->nodes);

	scf_node_t* root = e->nodes[0];

	if (scf_type_is_var(root->type)) {

		printf("%s(),%d, root: %p var: %p\n", __func__, __LINE__, root, root->var);

		root->result = scf_variable_clone(root->var);

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

static int _scf_op_semantic_create(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d, nb_nodes: %d\n", __func__, __LINE__, nb_nodes);

	assert(nb_nodes >= 1);

	scf_handler_data_t* d    = data;
	scf_variable_t**    pret = NULL;

	int ret;
	int i;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0 && SCF_FUNCTION_PTR == v0->type);

	scf_type_t* class  = scf_ast_find_type(ast, v0->w->text->data);
	assert(class);

	scf_vector_t* argv = scf_vector_alloc();

	for (i = 1; i < nb_nodes; i++) {

		pret    = d->pret;
		d->pret = &(nodes[i]->result);
		ret     = _scf_expr_calculate_internal(ast, nodes[i], d);
		d->pret = pret;
		SCF_CHECK_ERROR(ret < 0, -1, "calculate expr error\n");

		scf_vector_add(argv, nodes[i]->result);
	}

	printf("%s(),%d, nb_nodes: %d, argv->size: %d\n", __func__, __LINE__, nb_nodes, argv->size);

	scf_function_t* f = scf_scope_find_proper_function(class->scope, "init", argv);
	if (nb_nodes > 1) {
		SCF_CHECK_ERROR(!f, -1, "init function of class '%s' not found\n", v0->w->text->data);
	}
	v0->func_ptr = f;

	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(nodes[0]->parent->w , class, 0, 1, NULL);
	SCF_CHECK_ERROR(!r, -1, "return value alloc failed\n");

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;
	scf_variable_t*     v0 = _scf_operand_get(nodes[0]);
	scf_variable_t*     v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);
	assert(v0->type >= SCF_STRUCT);

	scf_type_t*		t = scf_ast_find_type_type(ast, v1->type);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v1->const_flag, v1->nb_pointers, v1->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (v0->nb_dimentions <= 0) {
		scf_loge("index out\n");
		return -1;
	}

	scf_handler_data_t* d    = data;
	scf_variable_t**	pret = d->pret;

	d->pret = &(nodes[1]->result);
	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* v1 = _scf_operand_get(nodes[1]);
	scf_variable_print(v1);
	assert(SCF_VAR_INT == v1->type);

	if (v0->dimentions[0] < 0) {
		scf_loge("\n");
		return -1;
	}

	if (v1->data.i < 0) {
		scf_loge("error: index %d < 0\n", v1->data.i);
		return -1;
	}

	if (v1->data.i >= v0->dimentions[0]) {
		scf_loge("index %d >= size %d\n", v1->data.i, v0->dimentions[0]);
		return -1;
	}

	scf_type_t*		t = scf_ast_find_type_type(ast, v0->type);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	int i;
	for (i = 1; i < v0->nb_dimentions; i++)
		scf_variable_add_array_dimention(r, v0->dimentions[i]);

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (0 == nb_nodes)
		return 0;

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);
	scf_logd("b: %p, nodes: %p, %d\n", b, b->node.nodes, b->node.nb_nodes);

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

		scf_operator_handler_t* h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			return -1;
		}

		scf_variable_t** pret = d->pret;

		d->pret = &node->result;
		int ret = h->func(ast, node->nodes, node->nb_nodes, d);
		d->pret = pret;

		if (ret < 0) {
			scf_loge("\n");
			ast->current_block = prev_block;
			return -1;
		}

		i++;
	}

	ast->current_block = prev_block;
	scf_logi("b: %p ok\n\n", b);
	return 0;
}

static int _scf_op_semantic_error(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;
	scf_variable_t**    pret = NULL;

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

		pret    = d->pret;
		d->pret = &(nodes[i]->result);
		int ret = _scf_expr_calculate_internal(ast, nodes[i], d);
		d->pret = pret;

		SCF_CHECK_ERROR(ret < 0, -1, "expr calculate error\n");
	}

	if (!scf_type_is_integer(nodes[0]->result->type)) {
		scf_loge("1st expr in error statement should got an interger for condition\n");
		return -1;
	}

	if (0 == nodes[1]->result->nb_pointers
			|| SCF_FUNCTION_PTR == nodes[1]->result->type) {
		scf_loge("2nd expr in error statement should got a pointer to be freed, but not a function pointer\n");
		return -1;
	}

	if (nb_nodes >= 3) {
		if (!scf_type_is_integer(nodes[2]->result->type)) {
			scf_loge("3rd expr in error statement should got an interger for return value\n");
			return -1;
		}
	} else if (nb_nodes < 3) {
		scf_logw("error in line: %d not set return value\n", nodes[0]->parent->w->line);
	}

	if (nb_nodes >= 4) {
		if (SCF_VAR_CHAR != nodes[3]->result->type
				|| 1 != nodes[3]->result->nb_pointers) {
			scf_loge("4th expr in error statement should got an format string\n");
			return -1;
		}
	}

	return 0;
}

static int _scf_op_semantic_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
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

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate(ast, e, &r) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("%s(),%d, r->type: %d, f->ret->type: %d\n", __func__, __LINE__, r->type, f->ret->type);

	if (r->type != f->ret->type || r->nb_pointers != f->ret->nb_pointers) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
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

	if (!n->parent) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_continue(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
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
	return 0;
}

static int _scf_op_semantic_label(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_semantic_goto(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* nl = nodes[0];
	assert(SCF_LABEL == nl->type);

	scf_label_t* l = nl->label;
	assert(l->w);

	scf_label_t* l2 = scf_block_find_label(ast->current_block, l->w->text->data);
	if (!l2) {
		printf("%s(),%d, error: label '%s' not found\n", __func__, __LINE__, l->w->text->data);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
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
	if (_scf_expr_calculate(ast, e, &r) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (!r || r->type != SCF_VAR_INT) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	scf_variable_free(r);
	r = NULL;

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

		scf_operator_handler_t* h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		scf_variable_t** pret = d->pret;

		d->pret = &node->result;
		int ret = h->func(ast, node->nodes, node->nb_nodes, d);
		d->pret = pret;

		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	printf("%s(),%d, ok: \n", __func__, __LINE__);
	return 0;
}

static int _scf_op_semantic_while(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	scf_logi("e: %p, b: %p, b->node.nodes: %p, b->node.nb_nodes: %d\n",
			e, b, b->node.nodes, b->node.nb_nodes);

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate(ast, e, &r) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (!r || r->type != SCF_VAR_INT) {
		scf_loge("\n");
		return -1;
	}
	scf_variable_free(r);
	r = NULL;

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

	scf_operator_handler_t*	h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t** pret = d->pret;

	d->pret = &node->result;
	int ret = h->func(ast, node->nodes, node->nb_nodes, d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_logi("ok\n");
	return 0;
}

static int __scf_op_semantic_call(scf_ast_t* ast, scf_function_t* f, void* data)
{
	scf_logi("f: %p, f->node->w: %s, f->ret->type: %d\n",
			f, f->node.w->text->data, f->ret->type);

	scf_handler_data_t* d = data;

	// save & change the current block
	scf_block_t* prev_block = ast->current_block;
	ast->current_block = (scf_block_t*)f;

	if (_scf_op_semantic_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (f->ret && d->pret) {
		*d->pret = scf_variable_clone(f->ret);
	}

	ast->current_block = prev_block;
	return 0;
}

int scf_function_semantic_analysis(scf_ast_t* ast, scf_function_t* f)
{
	scf_logi("f: %p\n", f);

	scf_handler_data_t d = {0};

	int ret = __scf_op_semantic_call(ast, f, &d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_logi("f: %p ok\n", f);
	return 0;
}

static int _scf_op_semantic_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(nb_nodes > 0);

	scf_handler_data_t* d    = data;
	scf_variable_t**    pret = d->pret;

	d->pret = &nodes[0]->result;
	int ret = _scf_expr_calculate_internal(ast, nodes[0], d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	if (SCF_FUNCTION_PTR != v0->type || !v0->func_ptr) {
		scf_loge("\n");
		return -1;
	}

	scf_function_t* f = v0->func_ptr;

	assert(f->argv->size == nb_nodes - 1);

	int i;
	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t* v0 = f->argv->data[i];
		scf_variable_t* v1 = _scf_operand_get(nodes[i + 1]);

		if (!scf_variable_same_type(v0, v1)) {
			scf_loge("arg var not same type, i: %d\n", i);
			return -1;
		}
	}

	if (d->pret && f->ret) {
		scf_type_t*     t = scf_ast_find_type_type(ast, f->ret->type);
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(nodes[0]->parent->w, t,
				f->ret->const_flag, f->ret->nb_pointers, f->ret->func_ptr);
		if (!r) {
			scf_loge("\n");
			return -1;
		}

		*d->pret = r;
	}

	scf_logw("f: %p ok\n", f);
	return 0;
}

static int _scf_op_semantic_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
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

static int _scf_op_semantic_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_type_is_number(v0->type)) {

		scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
		if (!r)
			return -ENOMEM;

		*d->pret = r;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_op_semantic_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_node_t* child  = nodes[0];
	scf_node_t* parent = nodes[0]->parent;

	nodes[0] = NULL;

	scf_node_free_data(parent);
	scf_node_move_data(parent, child);
	scf_node_free(child);

	return 0;
}

static int _scf_op_semantic_dereference(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	scf_logi("v0->nb_pointers: %d\n", v0->nb_pointers);

	if (v0->nb_pointers <= 0) {
		scf_loge("var is not a pointer\n");
		return -EINVAL;
	}

	scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers - 1, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (v0->const_literal_flag) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_type_t*	t = scf_ast_find_type_type(ast, v0->type);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers + 1, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* dst = _scf_operand_get(nodes[0]);
	scf_variable_t* src = _scf_operand_get(nodes[1]);

	assert(dst);
	assert(src);

	if (d->pret) {
		*d->pret = scf_variable_clone(dst);
	}

	return 0;
}

static int _scf_op_semantic_sizeof(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(SCF_OP_EXPR == nodes[0]->type);
	assert(d->pret == &parent->result);

	int size = v0->size;
	if (v0->nb_dimentions > 0) {
		int i;
		int capacity = 1;

		for (i = 0; i < v0->nb_dimentions; i++) {
			if (v0->dimentions[i] < 0) {
				scf_loge("\n");
				return -1;
			}

			capacity *= v0->dimentions[i];
		}

		v0->capacity = capacity;
		size = capacity * v0->size;
	}

	scf_type_t* t = scf_ast_find_type_type(ast, SCF_VAR_INT);

	scf_lex_word_t* w = parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, 1, 0, NULL);
	if (!r)
		return -ENOMEM;

	r->data.i     = size;
	r->const_flag = 1;

	SCF_XCHG(r->w, parent->w);

	scf_node_free_data(parent);
	parent->type = r->type;
	parent->var  = r;

	return 0;
}

static int _scf_op_semantic_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_type_is_integer(v0->type) || v0->nb_pointers > 0) {

		scf_type_t*	    t = scf_ast_find_type_type(ast, SCF_VAR_INT);

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, 0, NULL);
		if (!r)
			return -ENOMEM;

		*d->pret = r;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

static int _scf_op_semantic_bit_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	scf_type_t* t;

	if (v0->nb_pointers > 0) {
		t = scf_ast_find_type_type(ast, SCF_VAR_UINTPTR);

	} else if (scf_type_is_integer(v0->type)) {
		t = scf_ast_find_type_type(ast, v0->type);

	} else {
		scf_loge("\n");
		return -1;
	}

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, 0, NULL);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_binary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (scf_type_is_number(v0->type) && scf_type_is_number(v1->type)) {

		int             const_flag  = 0;
		int             nb_pointers = 0;
		scf_function_t* func_ptr    = NULL;

		scf_type_t* t = NULL;

		if (v0->nb_pointers > 0) {
			if (v1->nb_pointers > 0) {
				if (v0->type != v1->type || v0->nb_pointers != v1->nb_pointers) {
					scf_loge("different type pointer, type: %d,%d, nb_pointers: %d,%d\n",
							v0->type, v1->type, v0->nb_pointers, v1->nb_pointers);
					return -EINVAL;
				}
			} else if (!scf_type_is_integer(v1->type)) {
				scf_loge("var calculated with a pointer should be a interger, type: %d\n", v1->type);
				return -EINVAL;
			}

			t = scf_ast_find_type_type(ast, v0->type);

			const_flag  = v0->const_flag;
			nb_pointers = v0->nb_pointers;
			func_ptr    = v0->func_ptr;

		} else if (v1->nb_pointers > 0) {
			if (!scf_type_is_integer(v0->type)) {
				scf_loge("var calculated with a pointer should be a interger, type: %d\n", v0->type);
				return -EINVAL;
			}

			t = scf_ast_find_type_type(ast, v1->type);

			const_flag  = v1->const_flag;
			nb_pointers = v1->nb_pointers;
			func_ptr    = v1->func_ptr;

		} else if (v0->type == v1->type) { // from here v0 & v1 are normal var
			t = scf_ast_find_type_type(ast, v0->type);

			const_flag  = v0->const_flag && v1->const_flag;
			nb_pointers = 0;
			func_ptr    = NULL;

		} else {
			int ret = scf_find_updated_type(ast, v0, v1);
			if (ret < 0) {
				scf_loge("var type update failed, type: %d, %d\n", v0->type, v1->type);
				return -EINVAL;
			}

			t = scf_ast_find_type_type(ast, ret);

			if (t->type != v0->type)
				ret = _semantic_add_type_cast(ast, &nodes[0], v1, nodes[0]);
			else
				ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);

			if (ret < 0) {
				scf_loge("add type cast failed\n");
				return ret;
			}

			const_flag  = v0->const_flag && v1->const_flag;
			nb_pointers = 0;
			func_ptr    = NULL;
		}

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, nb_pointers, func_ptr);
		if (!r) {
			scf_loge("var alloc failed\n");
			return -ENOMEM;
		}

		*d->pret = r;
	} else {
		scf_loge("type %d, %d not support\n", v0->type, v1->type);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_bit(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (scf_type_is_integer(v0->type) || v0->nb_pointers > 0) {

		if (scf_type_is_integer(v1->type) || v1->nb_pointers > 0) {

			int const_flag = v0->const_flag && v1->const_flag;

			if (v0->type != v1->type || v0->nb_pointers != v1->nb_pointers) {

				int ret = _semantic_do_type_cast(ast, nodes, nb_nodes, data);
				if (ret < 0) {
					scf_loge("semantic do type cast failed\n");
					return ret;
				}
			}

			v0 = _scf_operand_get(nodes[0]);

			scf_type_t*     t = scf_ast_find_type_type(ast, v0->type);

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, 0, NULL);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
			return 0;
		}
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_bit_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_bit(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_bit_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_bit(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);

	if (v0->const_flag) {
		scf_loge("const var '%s' can't be assigned\n", v0->w->text->data);
		return -1;
	}

	if (v0->type != v1->type || v0->nb_pointers != v1->nb_pointers) {

		if (scf_type_cast_check(ast, v0, v1) < 0) {

			scf_type_t* t0 = scf_block_find_type_type(ast->current_block, v0->type);
			scf_type_t* t1 = scf_block_find_type_type(ast->current_block, v1->type);

			scf_loge("assign different type var, var '%s' is '%s', var '%s' is '%s'\n",
					v0->w->text->data, t0->name->data,
					v1->w->text->data, t1->name->data);
			return -1;
		}

		int ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			return ret;
		}
	}

	scf_type_t*	    t = scf_ast_find_type_type(ast, v0->type);

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
	if (!r) {
		scf_loge("var alloc failed\n");
		return -1;
	}

	*d->pret = r;
	return 0;
}


static int _scf_op_semantic_cmp(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);

	if (v0->nb_pointers > 0
			|| scf_type_is_integer(v0->type)
			|| scf_type_is_float(v0->type)) {

		if (v1->nb_pointers > 0
				|| scf_type_is_integer(v1->type)
				|| scf_type_is_float(v1->type)) {

			int const_flag = v0->const_flag && v1->const_flag; 

			if (v0->type != v1->type || v0->nb_pointers != v1->nb_pointers) {
				int ret = _semantic_do_type_cast(ast, nodes, nb_nodes, data);
				if (ret < 0) {
					scf_loge("semantic do type cast failed\n");
					return ret;
				}
			}
	
			v0 = _scf_operand_get(nodes[0]);

			scf_type_t*     t = scf_ast_find_type_type(ast, SCF_VAR_INT);

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, 0, NULL);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
			return 0;
		}
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_eq(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
#define CMPEQ_CHECK_FLOAT() \
	do { \
		assert(2 == nb_nodes); \
		scf_variable_t* v0 = _scf_operand_get(nodes[0]); \
		scf_variable_t* v1 = _scf_operand_get(nodes[1]); \
		if (scf_type_is_float(v0->type) || scf_type_is_float(v1->type)) { \
			scf_loge("float type can't cmp equal\n"); \
			return -EINVAL; \
		} \
	} while (0)

	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_ne(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_gt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_ge(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_lt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_le(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_logic(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);

	if (v0->nb_pointers > 0 || scf_type_is_integer(v0->type)) {

		if (v1->nb_pointers > 0 || scf_type_is_integer(v1->type)) {

			int const_flag = v0->const_flag && v1->const_flag;

			if (v0->type != v1->type || v0->nb_pointers != v1->nb_pointers) {

				int ret = _semantic_do_type_cast(ast, nodes, nb_nodes, data);
				if (ret < 0) {
					scf_loge("semantic do type cast failed\n");
					return ret;
				}
			}

			v0 = _scf_operand_get(nodes[0]);

			scf_type_t*     t = scf_ast_find_type_type(ast, SCF_VAR_INT);

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, 0, NULL);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
			return 0;
		}
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_logic_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_logic(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_logic_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_logic(ast, nodes, nb_nodes, data);
}

scf_operator_handler_t semantic_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,			-1, 	-1, -1, _scf_op_semantic_expr},
	{{NULL, NULL}, SCF_OP_CALL,			-1, 	-1, -1, _scf_op_semantic_call},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,	-1, 	-1, -1, _scf_op_semantic_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,      -1,     -1, -1, _scf_op_semantic_pointer},
	{{NULL, NULL}, SCF_OP_CREATE,       -1,     -1, -1, _scf_op_semantic_create},

	{{NULL, NULL}, SCF_OP_SIZEOF,       -1,     -1, -1, _scf_op_semantic_sizeof},
	{{NULL, NULL}, SCF_OP_TYPE_CAST,	-1, 	-1, -1, _scf_op_semantic_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,	-1, 	-1, -1, _scf_op_semantic_logic_not},
	{{NULL, NULL}, SCF_OP_BIT_NOT,      -1,     -1, -1, _scf_op_semantic_bit_not},
	{{NULL, NULL}, SCF_OP_NEG,			-1, 	-1, -1, _scf_op_semantic_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,		-1, 	-1, -1, _scf_op_semantic_positive},

	{{NULL, NULL}, SCF_OP_DEREFERENCE,	-1, 	-1, -1, _scf_op_semantic_dereference},
	{{NULL, NULL}, SCF_OP_ADDRESS_OF,	-1, 	-1, -1, _scf_op_semantic_address_of},

	{{NULL, NULL}, SCF_OP_MUL,			-1, 	-1, -1, _scf_op_semantic_mul},
	{{NULL, NULL}, SCF_OP_DIV,			-1, 	-1, -1, _scf_op_semantic_div},

	{{NULL, NULL}, SCF_OP_ADD,			-1, 	-1, -1, _scf_op_semantic_add},
	{{NULL, NULL}, SCF_OP_SUB,			-1, 	-1, -1, _scf_op_semantic_sub},

	{{NULL, NULL}, SCF_OP_BIT_AND,      -1,     -1, -1, _scf_op_semantic_bit_and},
	{{NULL, NULL}, SCF_OP_BIT_OR,       -1,     -1, -1, _scf_op_semantic_bit_or},

	{{NULL, NULL}, SCF_OP_EQ,			-1, 	-1, -1, _scf_op_semantic_eq},
	{{NULL, NULL}, SCF_OP_NE,           -1,     -1, -1, _scf_op_semantic_ne},
	{{NULL, NULL}, SCF_OP_GT,			-1, 	-1, -1, _scf_op_semantic_gt},
	{{NULL, NULL}, SCF_OP_LT,			-1, 	-1, -1, _scf_op_semantic_lt},
	{{NULL, NULL}, SCF_OP_GE,			-1, 	-1, -1, _scf_op_semantic_ge},
	{{NULL, NULL}, SCF_OP_LE,			-1, 	-1, -1, _scf_op_semantic_le},

	{{NULL, NULL}, SCF_OP_LOGIC_AND,    -1,     -1, -1, _scf_op_semantic_logic_and},
	{{NULL, NULL}, SCF_OP_LOGIC_OR,     -1,     -1, -1, _scf_op_semantic_logic_or},


	{{NULL, NULL}, SCF_OP_ASSIGN,		-1, 	-1, -1, _scf_op_semantic_assign},


	{{NULL, NULL}, SCF_OP_BLOCK,		-1, 	-1, -1, _scf_op_semantic_block},
	{{NULL, NULL}, SCF_OP_RETURN,		-1, 	-1, -1, _scf_op_semantic_return},
	{{NULL, NULL}, SCF_OP_BREAK,		-1, 	-1, -1, _scf_op_semantic_break},
	{{NULL, NULL}, SCF_OP_CONTINUE,		-1, 	-1, -1, _scf_op_semantic_continue},
	{{NULL, NULL}, SCF_OP_GOTO,			-1, 	-1, -1, _scf_op_semantic_goto},
	{{NULL, NULL}, SCF_LABEL,			-1, 	-1, -1, _scf_op_semantic_label},
	{{NULL, NULL}, SCF_OP_ERROR,        -1,     -1, -1, _scf_op_semantic_error},

	{{NULL, NULL}, SCF_OP_IF,			-1, 	-1, -1, _scf_op_semantic_if},
	{{NULL, NULL}, SCF_OP_WHILE,		-1, 	-1, -1, _scf_op_semantic_while},
};

scf_operator_handler_t* scf_find_semantic_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type)
{
	int i;
	for (i = 0; i < sizeof(semantic_operator_handlers) / sizeof(semantic_operator_handlers[0]); i++) {

		scf_operator_handler_t* h = &semantic_operator_handlers[i];

		if (type == h->type)
			return h;
	}

	return NULL;
}

