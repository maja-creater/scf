#include"scf_3ac.h"
#include"scf_graph.h"

static scf_3ac_operator_t _3ac_operators[] = {
	{SCF_OP_CALL, 			"call"},

	{SCF_OP_ARRAY_INDEX, 	"array_index"},
	{SCF_OP_POINTER,        "pointer"},

	{SCF_OP_TYPE_CAST, 	    "type_cast"},

	{SCF_OP_LOGIC_NOT, 		"not"},
	{SCF_OP_NEG, 			"neg"},
	{SCF_OP_POSITIVE, 		"positive"},

	{SCF_OP_DEREFERENCE,	"dereference"},
	{SCF_OP_ADDRESS_OF, 	"address_of"},

	{SCF_OP_MUL, 			"mul"},
	{SCF_OP_DIV, 			"div"},

	{SCF_OP_ADD, 			"add"},
	{SCF_OP_SUB, 			"sub"},

	{SCF_OP_EQ, 			"eq"},
	{SCF_OP_GT, 			"gt"},
	{SCF_OP_LT, 			"lt"},
//	{SCF_OP_GE, 			"ge"},
//	{SCF_OP_LE, 			"le"},

	{SCF_OP_ASSIGN, 		"assign"},

	{SCF_OP_RETURN,			"return"},
	{SCF_OP_GOTO,			"jmp"},

	{SCF_OP_3AC_TEQ,		"teq"},
	{SCF_OP_3AC_JE,			"jz"},
	{SCF_OP_3AC_JNE,		"jnz"},
	{SCF_OP_3AC_CALL_EXTERN, "call_extern"},
	{SCF_OP_3AC_PUSH,        "push"},
	{SCF_OP_3AC_POP,         "pop"},
};

scf_3ac_operator_t*	scf_3ac_find_operator(const int type)
{
	int i;
	for (i = 0; i < sizeof(_3ac_operators) / sizeof(_3ac_operators[0]); i++) {
		if (type == _3ac_operators[i].type) {
			return &(_3ac_operators[i]);
		}
	}

	return NULL;
}

scf_3ac_operand_t* scf_3ac_operand_alloc()
{
	scf_3ac_operand_t* operand = calloc(1, sizeof(operand));
	assert(operand);
	return operand;
}

void scf_3ac_operand_free(scf_3ac_operand_t* operand)
{
	if (operand) {
		free(operand);
		operand = NULL;
	}
}

scf_3ac_code_t* scf_3ac_code_alloc()
{
	scf_3ac_code_t* code = calloc(1, sizeof(scf_3ac_code_t));
	assert(code);

	code->active_vars	= scf_vector_alloc();
	return code;
}

void scf_3ac_code_free(scf_3ac_code_t* code)
{
	if (code) {
		if (code->dst)
			scf_3ac_operand_free(code->dst);

		if (code->srcs) {
			int i;
			for (i = 0; i < code->srcs->size; i++)
				scf_3ac_operand_free(code->srcs->data[i]);
			scf_vector_free(code->srcs);
		}

		scf_3ac_code_free(code);
		code = NULL;
	}
}

static void _3ac_print_node(scf_node_t* node)
{
	if (scf_type_is_var(node->type)) {
		printf("v_%d_%d/%s, ",
				node->var->w->line, node->var->w->pos, node->var->w->text->data);

	} else if (scf_type_is_operator(node->type)) {
		if (node->result) {
			printf("v_%d_%d/%s, ",
					node->result->w->line, node->result->w->pos, node->result->w->text->data);
		}
	} else if (SCF_FUNCTION == node->type) {
		printf("f_%d_%d/%s, ",
				node->w->line, node->w->pos, node->w->text->data);
	} else {
		assert(0);
	}
}

static void _3ac_print_dag_node(scf_dag_node_t* dag_node)
{
	if (scf_type_is_var(dag_node->type)) {
		printf("v_%d_%d/%s, ",
				dag_node->var->w->line, dag_node->var->w->pos, dag_node->var->w->text->data);

	} else if (scf_type_is_operator(dag_node->type)) {
		scf_3ac_operator_t* op = scf_3ac_find_operator(dag_node->type);
		printf("v_/%s, ", op->name);
	} else {
		assert(0);
	}
}

void _3ac_code_print(scf_list_t* h, scf_3ac_code_t* c)
{
	printf("%s  ", c->op->name);
	if (SCF_OP_3AC_CALL_EXTERN == c->op->type)
		printf("%s ", c->extern_fname->data);

	if (c->dst) {
		if (c->dst->node)
			_3ac_print_node(c->dst->node);

		else if (c->dst->dag_node) {
//			_3ac_print_dag_node(c->dst->dag_node);

		} else if (c->dst->code) {
			scf_list_t* l2 = scf_list_next(&c->dst->code->list);

			if (l2 != scf_list_sentinel(h)) {
				scf_3ac_code_t* c2 = scf_list_data(l2, scf_3ac_code_t, list);

				printf("c: ");
				_3ac_code_print(h, c2);
			} else
				printf("c: end");
		} else
			assert(0);
	}

	if (c->srcs) {
		int i;
		for (i = 0; i < c->srcs->size; i++) {
			scf_3ac_operand_t* src = c->srcs->data[i];
			if (src->node) {
				_3ac_print_node(src->node);

			} else if (src->dag_node) {
//				_3ac_print_dag_node(src->dag_node);

			} else if (src->code) {
				assert(0);
			}

			if (i < c->srcs->size - 1) {
				printf(", ");
			}
		}
	}
	printf("\n");
}

void scf_3ac_code_print(scf_list_t* h)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {
		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		_3ac_code_print(h, c);
	}
	printf("end\n");
}

int _scf_3ac_code_to_dag(scf_3ac_code_t* c, scf_list_t* dag)
{
#define SCF_3AC_GET_DAG_NODE(dag, node, dag_node) \
		scf_dag_node_t* dag_node = scf_list_find_dag_node((dag), (node)); \
		if (!dag_node) { \
			dag_node = scf_dag_node_alloc((node)->type, _scf_operand_get(node)); \
			scf_list_add_tail((dag), &dag_node->list); \
		}

	if (SCF_OP_ASSIGN == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];
		SCF_3AC_GET_DAG_NODE(dag, src->node, dag_src);
		src->dag_node = dag_src;

		SCF_3AC_GET_DAG_NODE(dag, c->dst->node, dag_dst);
		c->dst->dag_node = dag_dst;

		scf_dag_node_t* dag_assign = scf_dag_node_alloc(SCF_OP_ASSIGN, NULL);
		scf_dag_node_add_child(dag_assign, dag_dst);
		scf_dag_node_add_child(dag_assign, dag_src);
		scf_list_add_tail(dag, &dag_assign->list);

	} else if (SCF_OP_3AC_TEQ == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];
		SCF_3AC_GET_DAG_NODE(dag, src->node, dag_src);
		src->dag_node = dag_src;

	} else if (SCF_OP_RETURN  == c->op->type
			|| SCF_OP_3AC_POP == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];
		if (src->node) {
			SCF_3AC_GET_DAG_NODE(dag, src->node, dag_src);
			src->dag_node = dag_src;
		}
	} else {
#if 0
		printf("%s(),%d, c->op->name: %s\n", __func__, __LINE__, c->op->name);
		printf("%s(),%d, c->dst: %p\n", __func__, __LINE__, c->dst);
		printf("%s(),%d, c->dst->node: %p\n", __func__, __LINE__, c->dst->node);
		printf("%s(),%d, c->dst->node: %p, %p, %p\n", __func__, __LINE__, c->dst->node, c->dst->dag_node, c->dst->code);
#endif
		SCF_3AC_GET_DAG_NODE(dag, c->dst->node, dag_dst);
		c->dst->dag_node = dag_dst;

		int i;
		for (i = 0; i < c->srcs->size; i++) {
			scf_3ac_operand_t* src = c->srcs->data[i];

			SCF_3AC_GET_DAG_NODE(dag, src->node, dag_src);
			src->dag_node = dag_src;

			scf_dag_node_add_child(dag_dst, dag_src);
		}
	}
}

void scf_3ac_code_list_to_dag(scf_list_t* h, scf_list_t* dag)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {
		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		_scf_3ac_code_to_dag(c, dag);
	}
}

void scf_3ac_code_vector_to_dag(scf_vector_t* v, scf_list_t* dag)
{
	int i;
	for (i = 0; i < v->size; i++) {
		scf_3ac_code_t* c = v->data[i];

		_scf_3ac_code_to_dag(c, dag);
	}
}

int	scf_dag_expr_calculate(scf_list_t* h, scf_dag_node_t* node);
void scf_dag_to_3ac_code(scf_dag_node_t* dag, scf_list_t* h)
{
	scf_dag_expr_calculate(h, dag);
}

scf_3ac_context_t* scf_3ac_context_alloc()
{
	scf_3ac_context_t* _3ac_ctx = calloc(1, sizeof(scf_3ac_context_t));
	assert(_3ac_ctx);

	scf_list_init(&_3ac_ctx->_3ac_list_head);
	scf_list_init(&_3ac_ctx->dag_list_head);

	_3ac_ctx->basic_blocks			= scf_vector_alloc();
	return _3ac_ctx;
}

void scf_3ac_context_free(scf_3ac_context_t* _3ac_ctx)
{
	if (_3ac_ctx) {

		free(_3ac_ctx);
		_3ac_ctx = NULL;
	}
}

scf_3ac_basic_block_t* scf_3ac_basic_block_alloc()
{
	scf_3ac_basic_block_t* bb = calloc(1, sizeof(scf_3ac_basic_block_t));
	assert(bb);

	scf_list_init(&bb->dag_list_head);

	bb->codes		 	= scf_vector_alloc();
	bb->var_dag_nodes	= scf_vector_alloc();
	return bb;
}

scf_3ac_active_var_t* scf_3ac_active_var_alloc()
{
	scf_3ac_active_var_t* v = calloc(1, sizeof(scf_3ac_active_var_t));
	assert(v);
	return v;
}

void _scf_3ac_basic_blocks_print(scf_vector_t* basic_blocks, scf_list_t* h)
{
	int i;
	int j;
	for (i = 0; i < basic_blocks->size; i++) {
		scf_3ac_basic_block_t* bb = basic_blocks->data[i];
		printf("\n\033[34mbasic_block:\033[0m\n");

		for (j = 0; j < bb->codes->size; j++) {
			scf_3ac_code_t* c = bb->codes->data[j];
			_3ac_code_print(h, c);
		}
	}
}

void _scf_3ac_basic_blocks_find(scf_3ac_context_t* _3ac_ctx, scf_list_t* h)
{
	int			start = 0;
	scf_list_t*	l;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		scf_list_t*		l2 = NULL;
		scf_3ac_code_t*	n2 = NULL;

		if (!start) {
			c->basic_block_start = 1;
			start = 1;
		}

		if (SCF_OP_CALL == c->op->type
				|| SCF_OP_3AC_CALL_EXTERN == c->op->type) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
			continue;
		}

		if (SCF_OP_GOTO == c->op->type
				|| SCF_OP_3AC_JE == c->op->type
				|| SCF_OP_3AC_JNE == c->op->type) {

			assert(c->dst->code);

			l2 = scf_list_next(&c->dst->code->list);
			if (l2 != scf_list_sentinel(h)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
		}
	}


	scf_3ac_basic_block_t* bb = NULL;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (c->basic_block_start) {
			if (bb) {
				scf_vector_add(_3ac_ctx->basic_blocks, bb);
				bb = NULL;
			}

			bb = scf_3ac_basic_block_alloc();
			scf_vector_add(bb->codes, c);
		
			if (SCF_OP_CALL == c->op->type || SCF_OP_3AC_CALL_EXTERN == c->op->type)
				bb->call_flag = 1;

			if (SCF_OP_GOTO == c->op->type
					|| SCF_OP_3AC_JE == c->op->type
					|| SCF_OP_3AC_JNE == c->op->type)
				bb->jmp_flag = 1;

		} else {
			assert(bb);
			scf_vector_add(bb->codes, c);
		}
	}

	if (bb) {
		scf_vector_add(_3ac_ctx->basic_blocks, bb);
	}
}

int scf_3ac_copy_active_vars(scf_vector_t* dag_nodes, scf_vector_t* active_vars)
{
	int j;
	for (j = 0; j < dag_nodes->size; j++) {
		scf_dag_node_t* dag_node = dag_nodes->data[j];

		if (dag_node->active) {
			scf_3ac_active_var_t* v = scf_3ac_active_var_alloc();
			v->dag_node	= dag_node;
			v->active	= 1;
			scf_vector_add(active_vars, v);
		}
	}
	return 0;
}

int scf_3ac_bb_find_active_vars(scf_3ac_basic_block_t* bb, scf_list_t* h)
{
	scf_3ac_code_vector_to_dag(bb->codes, &bb->dag_list_head);
	
	scf_vector_t* roots = scf_vector_alloc();
	scf_vector_t* leafs = scf_vector_alloc();

	scf_dag_find_roots(&bb->dag_list_head, roots);

	scf_dag_vector_find_leafs(roots, leafs);

	int i;
	for (i = 0; i < leafs->size; i++) {
		scf_dag_node_t* dag_node = leafs->data[i];

		printf("%s(),%d, dag_node: %p, %s\n", __func__, __LINE__, dag_node, dag_node->var->w->text->data);

		// it should be 'real' variable, not 'constant literal value' saved in variable
		if (!dag_node->var->const_literal_flag) {
			dag_node->active = 1;
			scf_vector_add(bb->var_dag_nodes, dag_node);
		}
	}

	for (i = bb->codes->size - 1; i >= 0; i--) {
		scf_3ac_code_t* c = bb->codes->data[i];

//		_3ac_code_print(h, c);

		if (c->dst) {
			if (c->dst->dag_node)
				c->dst->dag_node->active = 0;
		}

		int j;
		for (j = 0; j < c->srcs->size; j++) {
			scf_3ac_operand_t* src = c->srcs->data[j];

			if (!src->node)
				continue;

			src->dag_node->active = 1;

			printf("%s(),%d, dag_node: %p, %s\n", __func__, __LINE__, src->dag_node, src->dag_node->var->w->text->data);

			if (scf_type_is_operator(src->dag_node->type)
					|| !src->dag_node->var->const_literal_flag) {

				if (!scf_vector_find(bb->var_dag_nodes, src->dag_node))
					scf_vector_add(bb->var_dag_nodes, src->dag_node);
			}
		}

		scf_3ac_copy_active_vars(bb->var_dag_nodes, c->active_vars);
	}

	return 0;
}

static int _scf_graph_cmp_dag_node(const void* v0, const void* v1)
{
	const scf_dag_node_t*	dn0 = v0;
	const scf_graph_node_t*	gn1 = v1;
	return !(dn0 == gn1->data);
}

void scf_3ac_colored_graph_print(scf_graph_t* graph)
{
	int i;
	for (i = 0; i < graph->nodes->size; i++) {

		scf_graph_node_t*	gn		 = graph->nodes->data[i];
		scf_dag_node_t*		dag_node = gn->data;

		printf("v_%d_%d/%s, color: %d\n",
				dag_node->var->w->line, dag_node->var->w->pos, dag_node->var->w->text->data,
				gn->color);
	}
}

int scf_3ac_code_make_register_conflict_graph(scf_3ac_code_t* c, scf_graph_t* graph)
{
	scf_graph_node_t* gn0 = scf_vector_find_cmp(graph->nodes, c->dst->dag_node, _scf_graph_cmp_dag_node);
	if (!gn0) {
		gn0 		= scf_graph_node_alloc();
		gn0->data	= c->dst->dag_node;
		scf_graph_add_node(graph, gn0);
	}

	int j;
	for (j = 0; j < c->active_vars->size; j++) {
		scf_3ac_active_var_t* v = c->active_vars->data[j];

		scf_graph_node_t* gn1 = scf_vector_find_cmp(graph->nodes, v->dag_node, _scf_graph_cmp_dag_node);
		if (!gn1) {
			gn1 		= scf_graph_node_alloc();
			gn1->data	= v->dag_node;
			scf_graph_add_node(graph, gn1);
		}

		if (scf_vector_find(gn0->neighbors, gn1)) {
			assert(scf_vector_find(gn1->neighbors, gn0));
		} else {
			scf_graph_make_edge(gn0, gn1);
			scf_graph_make_edge(gn1, gn0);
		}
	}
	return 0;
}

int scf_3ac_make_register_conflict_graph(scf_vector_t* codes, scf_graph_t** pgraph)
{
	scf_graph_t* graph = scf_graph_alloc();

	int i;
	for (i = 0; i < codes->size; i++) {
		scf_3ac_code_t* c = codes->data[i];

		scf_3ac_code_make_register_conflict_graph(c, graph);
	}

	printf("%s(),%d, graph->nodes->size: %d\n", __func__, __LINE__, graph->nodes->size);

	*pgraph = graph;
	return 0;
}

int scf_3ac_filter(scf_3ac_context_t* _3ac_ctx, scf_list_t* _3ac_dst_list, scf_list_t* _3ac_src_list)
{
	_scf_3ac_basic_blocks_find(_3ac_ctx, _3ac_src_list);

	_scf_3ac_basic_blocks_print(_3ac_ctx->basic_blocks, _3ac_src_list);
#if 1
	int i;
	for (i = 0; i < _3ac_ctx->basic_blocks->size; i++) {
		scf_3ac_basic_block_t* bb = _3ac_ctx->basic_blocks->data[i];

		if (bb->call_flag || bb->jmp_flag)
			continue;

		scf_graph_t* graph = NULL;

		scf_3ac_bb_find_active_vars(bb, _3ac_src_list);
	}
#endif
	return 0;
}

