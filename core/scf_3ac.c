#include"scf_3ac.h"
#include"scf_function.h"
#include"scf_basic_block.h"
#include"scf_graph.h"

static scf_3ac_operator_t _3ac_operators[] = {
	{SCF_OP_CALL, 			"call"},

	{SCF_OP_ARRAY_INDEX, 	"array_index"},
	{SCF_OP_POINTER,        "pointer"},

	{SCF_OP_TYPE_CAST, 	    "cast"},

	{SCF_OP_LOGIC_NOT, 		"logic_not"},
	{SCF_OP_BIT_NOT,        "not"},
	{SCF_OP_NEG, 			"neg"},
	{SCF_OP_POSITIVE, 		"positive"},

	{SCF_OP_DEREFERENCE,	"dereference"},
	{SCF_OP_ADDRESS_OF, 	"address_of"},

	{SCF_OP_MUL, 			"mul"},
	{SCF_OP_DIV, 			"div"},

	{SCF_OP_ADD, 			"add"},
	{SCF_OP_SUB, 			"sub"},

	{SCF_OP_BIT_AND,        "and"},
	{SCF_OP_BIT_OR,         "or"},

	{SCF_OP_EQ, 			"eq"},
	{SCF_OP_NE,             "neq"},
	{SCF_OP_GT, 			"gt"},
	{SCF_OP_LT, 			"lt"},
	{SCF_OP_GE, 			"ge"},
	{SCF_OP_LE, 			"le"},

	{SCF_OP_ASSIGN, 		 "assign"},

	{SCF_OP_RETURN,			 "return"},
	{SCF_OP_GOTO,			 "jmp"},

	{SCF_OP_3AC_TEQ,         "teq"},
	{SCF_OP_3AC_CMP,         "cmp"},

	{SCF_OP_3AC_SETZ,        "setz"},
	{SCF_OP_3AC_SETNZ,       "setnz"},

	{SCF_OP_3AC_DEREFERENCE_LV,	"dereference_lv"},
	{SCF_OP_3AC_ARRAY_INDEX_LV,	"array_index_lv"},

	{SCF_OP_3AC_JZ,          "jz"},
	{SCF_OP_3AC_JNZ,         "jnz"},
	{SCF_OP_3AC_JGT,         "jgt"},
	{SCF_OP_3AC_JGE,         "jge"},
	{SCF_OP_3AC_JLT,         "jlt"},
	{SCF_OP_3AC_JLE,         "jle"},

	{SCF_OP_3AC_CALL_EXTERN, "call_extern"},
	{SCF_OP_3AC_PUSH,        "push"},
	{SCF_OP_3AC_POP,         "pop"},
	{SCF_OP_3AC_SAVE,        "save"},
	{SCF_OP_3AC_LOAD,        "load"},
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
	scf_3ac_code_t* c = calloc(1, sizeof(scf_3ac_code_t));

	return c;
}

void scf_3ac_code_free(scf_3ac_code_t* c)
{
	if (c) {
		if (c->dst)
			scf_3ac_operand_free(c->dst);

		if (c->srcs) {
			int i;
			for (i = 0; i < c->srcs->size; i++)
				scf_3ac_operand_free(c->srcs->data[i]);
			scf_vector_free(c->srcs);
		}

		if (c->extern_fname)
			scf_string_free(c->extern_fname);

		if (c->active_vars) {
			int i;
			for (i = 0; i < c->active_vars->size; i++)
				scf_active_var_free(c->active_vars->data[i]);
			scf_vector_free(c->active_vars);
		}

		free(c);
		c = NULL;
	}
}

scf_3ac_code_t* scf_3ac_alloc_by_src(int op_type, scf_dag_node_t* src)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("3ac operator not found\n");
		return NULL;
	}

	scf_3ac_operand_t* src0	= scf_3ac_operand_alloc();
	if (!src0)
		return NULL;
	src0->dag_node = src;

	scf_vector_t* srcs = scf_vector_alloc();
	if (!srcs)
		goto error1;
	if (scf_vector_add(srcs, src0) < 0)
		goto error0;

	scf_3ac_code_t* c = scf_3ac_code_alloc();
	if (!c)
		goto error0;

	c->op	= _3ac_op;
	c->srcs	= srcs;
	return c;

error0:
	scf_vector_free(srcs);
error1:
	scf_3ac_operand_free(src0);
	return NULL;
}

scf_3ac_code_t* scf_3ac_alloc_by_dst(int op_type, scf_dag_node_t* dst)
{
	scf_3ac_operator_t* _3ac_op = scf_3ac_find_operator(op_type);
	if (!_3ac_op) {
		scf_loge("3ac operator not found\n");
		return NULL;
	}

	scf_3ac_operand_t* d = scf_3ac_operand_alloc();
	if (!d)
		return NULL;
	d->dag_node = dst;

	scf_3ac_code_t* c= scf_3ac_code_alloc();
	if (!c) {
		scf_3ac_operand_free(d);
		return NULL;
	}

	c->op	= _3ac_op;
	c->dst	= d;
	return c;
}

static scf_3ac_code_t* _scf_branch_ops_code(int type, scf_label_t* l, scf_node_t* err)
{
	scf_3ac_code_t* branch = scf_3ac_code_alloc();

	branch->label	= l;
	branch->error   = err;
	branch->dst		= scf_3ac_operand_alloc();
	branch->op		= scf_3ac_find_operator(type);

}

static void _3ac_print_node(scf_node_t* node)
{
	if (scf_type_is_var(node->type)) {
		if (node->var->w) {
			printf("v_%d_%d/%s",
					node->var->w->line, node->var->w->pos, node->var->w->text->data);
		} else {
			printf("v_%#lx", 0xffff & (uintptr_t)node->var);
		}
	} else if (scf_type_is_operator(node->type)) {
		if (node->result) {
			if (node->result->w) {
				printf("v_%d_%d/%s",
						node->result->w->line, node->result->w->pos, node->result->w->text->data);
			} else
				printf("v/%#lx", 0xffff & (uintptr_t)node->result);
		}
	} else if (SCF_FUNCTION == node->type) {
		printf("f_%d_%d/%s",
				node->w->line, node->w->pos, node->w->text->data);
	} else {
		scf_loge("node: %p\n", node);
		scf_loge("node->type: %d\n", node->type);
		assert(0);
	}
}

static void _3ac_print_dag_node(scf_dag_node_t* dn)
{
	if (scf_type_is_var(dn->type)) {
		if (dn->var->w) {
			printf("v_%d_%d/%s, ",
					dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
		} else {
			printf("v_%#lx", 0xffff & (uintptr_t)dn->var);
		}
	} else if (scf_type_is_operator(dn->type)) {
		scf_3ac_operator_t* op = scf_3ac_find_operator(dn->type);
		printf("v_/%s, ", op->name);
	} else {
		assert(0);
	}
}

void scf_3ac_code_print(scf_3ac_code_t* c, scf_list_t* sentinel)
{
	printf("%s  ", c->op->name);
	if (SCF_OP_3AC_CALL_EXTERN == c->op->type)
		printf("%s ", c->extern_fname->data);

	if (c->dst) {
		if (c->dst->node)
			_3ac_print_node(c->dst->node);

		else if (c->dst->dag_node) {
			_3ac_print_dag_node(c->dst->dag_node);

		} else if (c->dst->code) {
			if (&c->dst->code->list != sentinel) {
				printf(": ");
				scf_3ac_code_print(c->dst->code, sentinel);
			} else
				printf(": end");
		}
	}

	if (c->srcs) {
		int i;
		for (i = 0; i < c->srcs->size; i++) {
			if (0 == i && c->dst && c->dst->node)
				printf(", ");

			scf_3ac_operand_t* src = c->srcs->data[i];
			if (src->node) {
				_3ac_print_node(src->node);

			} else if (src->dag_node) {
				_3ac_print_dag_node(src->dag_node);

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

int scf_3ac_code_to_dag(scf_3ac_code_t* c, scf_list_t* dag)
{
	int ret = 0;

	if (SCF_OP_ASSIGN == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_get_node(dag, c->dst->node, &c->dst->dag_node);
		if (ret < 0)
			return ret;

		scf_dag_node_t* dag_assign = scf_dag_node_alloc(SCF_OP_ASSIGN, NULL);
		scf_list_add_tail(dag, &dag_assign->list);

		ret = scf_dag_node_add_child(dag_assign, c->dst->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_node_add_child(dag_assign, src->dag_node);
		if (ret < 0)
			return ret;

	} else if (SCF_OP_3AC_TEQ == c->op->type
			|| SCF_OP_RETURN == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];

		if (src->node) {
			ret = scf_dag_get_node(dag, src->node, &src->dag_node);
			if (ret < 0)
				return ret;
		}
	} else if (scf_type_is_jmp(c->op->type)) {

		scf_logi("c->op: %d, name: %s\n", c->op->type, c->op->name);

	} else {
		scf_dag_node_t* dag_dst = NULL;

		if (c->dst && c->dst->node) {
			ret = scf_dag_get_node(dag, c->dst->node, &c->dst->dag_node);
			if (ret < 0)
				return ret;

			dag_dst = c->dst->dag_node;
		}

		if (c->srcs) {
			int i;
			for (i = 0; i < c->srcs->size; i++) {

				scf_3ac_operand_t* src = c->srcs->data[i];
				if (!src || !src->node)
					continue;

				ret = scf_dag_get_node(dag, src->node, &src->dag_node);
				if (ret < 0)
					return ret;

				if (dag_dst) {
					ret = scf_dag_node_add_child(dag_dst, src->dag_node);
					if (ret < 0)
						return ret;
				}
			}
		}
	}

	return 0;
}

int scf_3ac_split_basic_blocks(scf_list_t* list_head_3ac, scf_function_t* f)
{
	int			start = 0;
	scf_list_t*	l;

	for (l = scf_list_head(list_head_3ac); l != scf_list_sentinel(list_head_3ac);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		scf_list_t*		l2 = NULL;
		scf_3ac_code_t*	n2 = NULL;

		if (!start) {
			c->basic_block_start = 1;
			start = 1;
		}

		if (SCF_OP_CALL == c->op->type
				|| SCF_OP_RETURN == c->op->type
				|| SCF_OP_3AC_CALL_EXTERN == c->op->type) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(list_head_3ac)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
			continue;
		}

		if (scf_type_is_jmp(c->op->type)) {
			assert(c->dst->code);

			l2 = &c->dst->code->list;
			if (l2 != scf_list_sentinel(list_head_3ac)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(list_head_3ac)) {
				n2 = scf_list_data(l2, scf_3ac_code_t, list);
				n2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
		}
	}

	scf_basic_block_t* bb = NULL;

	for (l = scf_list_head(list_head_3ac); l != scf_list_sentinel(list_head_3ac); ) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		l = scf_list_next(l);

		if (c->basic_block_start) {

			bb = scf_basic_block_alloc();
			if (!bb) {
				scf_loge("basic block alloc failed\n");
				return -ENOMEM;
			}
			bb->index = f->nb_basic_blocks++;
			scf_list_add_tail(&f->basic_block_list_head, &bb->list);

			c->basic_block = bb;

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);

			if (SCF_OP_CALL == c->op->type || SCF_OP_3AC_CALL_EXTERN == c->op->type)
				bb->call_flag = 1;

			if (SCF_OP_RETURN == c->op->type)
				bb->ret_flag = 1;

			if (scf_type_is_jmp(c->op->type)) {
				bb->jmp_flag = 1;

				int ret = scf_vector_add(f->jmps, c);
				if (ret < 0) {
					scf_loge("\n");
					return -ret;
				}
			}
		} else {
			assert(bb);
			c->basic_block = bb;

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);
		}
	}

	int i;
	int ret;
	for (i = 0; i < f->jmps->size; i++) {
		scf_3ac_code_t* c   = f->jmps->data[i];
		scf_3ac_code_t* dst = c->dst->code;

		scf_basic_block_t* current_bb = c->basic_block;
		scf_basic_block_t* dst_bb     = dst->basic_block;
		scf_basic_block_t* prev_bb    = NULL;

		l = scf_list_prev(&current_bb->list);
		if (l != scf_list_sentinel(&f->basic_block_list_head))
			prev_bb = scf_list_data(l, scf_basic_block_t, list);

		if (prev_bb) {
			ret = scf_vector_add(prev_bb->nexts, dst_bb);
			if (ret < 0) {
				scf_loge("\n");
				return -ret;
			}

			ret = scf_vector_add(dst_bb->prevs, prev_bb);
			if (ret < 0) {
				scf_loge("\n");
				return -ret;
			}
		}
	}

	return 0;
}

