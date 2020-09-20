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

	{SCF_OP_INC,            "inc"},
	{SCF_OP_DEC,            "dec"},

	{SCF_OP_INC_POST,       "inc_post"},
	{SCF_OP_DEC_POST,       "dec_post"},

	{SCF_OP_DEREFERENCE,	"dereference"},
	{SCF_OP_ADDRESS_OF, 	"address_of"},

	{SCF_OP_MUL, 			"mul"},
	{SCF_OP_DIV, 			"div"},
	{SCF_OP_MOD,            "mod"},

	{SCF_OP_ADD, 			"add"},
	{SCF_OP_SUB, 			"sub"},

	{SCF_OP_SHL,            "shl"},
	{SCF_OP_SHR,            "shr"},

	{SCF_OP_BIT_AND,        "and"},
	{SCF_OP_BIT_OR,         "or"},

	{SCF_OP_EQ, 			"eq"},
	{SCF_OP_NE,             "neq"},
	{SCF_OP_GT, 			"gt"},
	{SCF_OP_LT, 			"lt"},
	{SCF_OP_GE, 			"ge"},
	{SCF_OP_LE, 			"le"},

	{SCF_OP_ASSIGN,         "assign"},
	{SCF_OP_ADD_ASSIGN,     "+="},
	{SCF_OP_SUB_ASSIGN,     "-="},
	{SCF_OP_MUL_ASSIGN,     "*="},
	{SCF_OP_DIV_ASSIGN,     "/="},
	{SCF_OP_MOD_ASSIGN,     "%="},
	{SCF_OP_SHL_ASSIGN,     "<<="},
	{SCF_OP_SHR_ASSIGN,     ">>="},
	{SCF_OP_AND_ASSIGN,     "&="},
	{SCF_OP_OR_ASSIGN,      "|="},

	{SCF_OP_RETURN,			 "return"},
	{SCF_OP_GOTO,			 "jmp"},

	{SCF_OP_3AC_TEQ,         "teq"},
	{SCF_OP_3AC_CMP,         "cmp"},

	{SCF_OP_3AC_LEA,         "lea"},

	{SCF_OP_3AC_SETZ,        "setz"},
	{SCF_OP_3AC_SETNZ,       "setnz"},

	{SCF_OP_3AC_JZ,          "jz"},
	{SCF_OP_3AC_JNZ,         "jnz"},
	{SCF_OP_3AC_JGT,         "jgt"},
	{SCF_OP_3AC_JGE,         "jge"},
	{SCF_OP_3AC_JLT,         "jlt"},
	{SCF_OP_3AC_JLE,         "jle"},

	{SCF_OP_3AC_NOP,         "nop"},
	{SCF_OP_3AC_END,         "end"},

	{SCF_OP_3AC_CALL_EXTERN, "call_extern"},
	{SCF_OP_3AC_PUSH,        "push"},
	{SCF_OP_3AC_POP,         "pop"},
	{SCF_OP_3AC_SAVE,        "save"},
	{SCF_OP_3AC_LOAD,        "load"},
	{SCF_OP_3AC_RELOAD,      "reload"},
	{SCF_OP_3AC_RESAVE,      "resave"},

	{SCF_OP_3AC_ASSIGN_DEREFERENCE,	    "dereference="},
	{SCF_OP_3AC_ASSIGN_ARRAY_INDEX,	    "array_index="},
	{SCF_OP_3AC_ASSIGN_POINTER,	        "pointer="},

	{SCF_OP_3AC_ADD_ASSIGN_DEREFERENCE,	"dereference+="},
	{SCF_OP_3AC_ADD_ASSIGN_ARRAY_INDEX,	"array_index+="},
	{SCF_OP_3AC_ADD_ASSIGN_POINTER,	    "pointer+="},

	{SCF_OP_3AC_SUB_ASSIGN_DEREFERENCE,	"dereference-="},
	{SCF_OP_3AC_SUB_ASSIGN_ARRAY_INDEX,	"array_index-="},
	{SCF_OP_3AC_SUB_ASSIGN_POINTER,	    "pointer-="},

	{SCF_OP_3AC_AND_ASSIGN_DEREFERENCE, "dereference&="},
	{SCF_OP_3AC_AND_ASSIGN_ARRAY_INDEX, "array_index&="},
	{SCF_OP_3AC_AND_ASSIGN_POINTER,     "pointer&="},

	{SCF_OP_3AC_OR_ASSIGN_DEREFERENCE,  "dereference|="},
	{SCF_OP_3AC_OR_ASSIGN_ARRAY_INDEX,  "array_index|="},
	{SCF_OP_3AC_OR_ASSIGN_POINTER,      "pointer|="},

	{SCF_OP_3AC_INC_DEREFERENCE,        "++dereference"},
	{SCF_OP_3AC_INC_ARRAY_INDEX,        "++array_index"},
	{SCF_OP_3AC_INC_POINTER,            "++pointer"},

	{SCF_OP_3AC_DEC_DEREFERENCE,        "--dereference"},
	{SCF_OP_3AC_DEC_ARRAY_INDEX,        "--array_index"},
	{SCF_OP_3AC_DEC_POINTER,            "--pointer"},

	{SCF_OP_3AC_INC_POST_DEREFERENCE,   "dereference++"},
	{SCF_OP_3AC_INC_POST_ARRAY_INDEX,   "array_index++"},
	{SCF_OP_3AC_INC_POST_POINTER,       "pointer++"},

	{SCF_OP_3AC_DEC_POST_DEREFERENCE,   "dereference--"},
	{SCF_OP_3AC_DEC_POST_ARRAY_INDEX,   "array_index--"},
	{SCF_OP_3AC_DEC_POST_POINTER,       "pointer--"},

	{SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX, "&array_index"},
	{SCF_OP_3AC_ADDRESS_OF_POINTER,     "&pointer"},
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
	scf_3ac_operand_t* operand = calloc(1, sizeof(scf_3ac_operand_t));
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

int scf_3ac_code_same(scf_3ac_code_t* c0, scf_3ac_code_t* c1)
{
	if (c0->op != c1->op)
		return 0;

	if (c0->dst) {
		if (!c1->dst)
			return 0;

		if (c0->dst->dag_node) {
			if (!c1->dst->dag_node)
				return 0;

			if (!scf_dag_dn_same(c0->dst->dag_node, c1->dst->dag_node))
				return 0;
		} else if (c1->dst->dag_node)
			return 0;
	} else if (c1->dst)
		return 0;

	if (c0->srcs) {
		if (!c1->srcs)
			return 0;

		if (c0->srcs->size != c1->srcs->size)
			return 0;

		int i;
		for (i = 0; i < c0->srcs->size; i++) {
			scf_3ac_operand_t* src0 = c0->srcs->data[i];
			scf_3ac_operand_t* src1 = c1->srcs->data[i];

			if (src0->dag_node) {
				if (!src1->dag_node)
					return 0;

				if (!scf_dag_dn_same(src0->dag_node, src1->dag_node))
					return 0;
			} else if (src1->dag_node)
				return 0;
		}
	} else if (c1->srcs)
		return 0;
	return 1;
}

scf_3ac_code_t* scf_3ac_code_clone(scf_3ac_code_t* c)
{
	scf_3ac_code_t* c2 = calloc(1, sizeof(scf_3ac_code_t));
	if (!c2)
		return NULL;

	c2->op = c->op;

	if (c->dst) {
		c2->dst = scf_3ac_operand_alloc();
		if (!c2->dst) {
			free(c2);
			return NULL;
		}

		c2->dst->node     = c->dst->node;
		c2->dst->dag_node = c->dst->dag_node;
		c2->dst->code     = c->dst->code;
	}

	if (c->srcs) {
		c2->srcs = scf_vector_alloc();
		if (!c2->srcs) {
			scf_3ac_code_free(c2);
			return NULL;
		}

		int i;
		for (i = 0; i < c->srcs->size; i++) {
			scf_3ac_operand_t* src  = c->srcs->data[i];
			scf_3ac_operand_t* src2 = scf_3ac_operand_alloc();

			if (!src2) {
				scf_3ac_code_free(c2);
				return NULL;
			}

			if (scf_vector_add(c2->srcs, src2) < 0) {
				scf_3ac_code_free(c2);
				return NULL;
			}

			src2->node     = src->node;
			src2->dag_node = src->dag_node;
			src2->code     = src->code;
		}
	}

	c2->label = c->label;
	c2->error = c->error;

	if (c->extern_fname) {
		c2->extern_fname = scf_string_clone(c->extern_fname);
		if (!c2->extern_fname) {
			scf_3ac_code_free(c2);
			return NULL;
		}
	}

	return c2;
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
		if (dn->var && dn->var->w)
			printf("v_%d_%d/%s, ", dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
		else
			printf("v_/%s, ", op->name);
	} else {
		//printf("type: %d, v_%#lx\n", dn->type, 0xffff & (uintptr_t)dn->var);
		//assert(0);
	}
}

void scf_3ac_code_print(scf_3ac_code_t* c, scf_list_t* sentinel)
{
	printf("%s  ", c->op->name);
	if (SCF_OP_3AC_CALL_EXTERN == c->op->type)
		printf("%s ", c->extern_fname->data);

	if (c->dst) {
		if (c->dst->dag_node) {
			_3ac_print_dag_node(c->dst->dag_node);

		} else if (c->dst->node) {
			_3ac_print_node(c->dst->node);

		} else if (c->dst->code) {
			if (&c->dst->code->list != sentinel) {
				printf(": ");
				scf_3ac_code_print(c->dst->code, sentinel);
			} else {
				//printf(": end");
			}
		} else if (c->dst->bb) {
			printf(" bb: %p ", c->dst->bb);
		}
	}

	if (c->srcs) {
		int i;
		for (i = 0; i < c->srcs->size; i++) {
			if (0 == i && c->dst && c->dst->node)
				printf(", ");

			scf_3ac_operand_t* src = c->srcs->data[i];

			if (src->dag_node) {
				_3ac_print_dag_node(src->dag_node);

			} else if (src->node) {
				_3ac_print_node(src->node);

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

	if (scf_type_is_assign(c->op->type)) {

		scf_3ac_operand_t* src = c->srcs->data[0];

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_get_node(dag, c->dst->node, &c->dst->dag_node);
		if (ret < 0)
			return ret;

		assert(c->dst->node->parent);
		scf_variable_t* var_assign = _scf_operand_get(c->dst->node->parent);

		scf_dag_node_t* dag_assign = scf_dag_node_alloc(c->op->type, var_assign);
		scf_list_add_tail(dag, &dag_assign->list);

		ret = scf_dag_node_add_child(dag_assign, c->dst->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_node_add_child(dag_assign, src->dag_node);
		if (ret < 0)
			return ret;

	} else if (SCF_OP_INC == c->op->type
			|| SCF_OP_INC_POST == c->op->type
			|| SCF_OP_DEC == c->op->type
			|| SCF_OP_DEC_POST == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];

		assert(src->node->parent);
		scf_variable_t*    var_parent = _scf_operand_get(src->node->parent);
		scf_dag_node_t*    dn_parent  = scf_dag_node_alloc(c->op->type, var_parent);
		scf_list_add_tail(dag, &dn_parent->list);

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_node_add_child(dn_parent, src->dag_node);
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

		scf_logd("c->op: %d, name: %s\n", c->op->type, c->op->name);

	} else {
		scf_dag_node_t* dag_dst = NULL;
		scf_operator_t* op      = NULL;

		if (c->dst && c->dst->node) {
			ret = scf_dag_get_node(dag, c->dst->node, &c->dst->dag_node);
			if (ret < 0)
				return ret;

			dag_dst = c->dst->dag_node;
			op      = c->dst->node->op;
			assert(op);
		}

		if (!c->srcs)
			return 0;

		int i;
		for (i = 0; i < c->srcs->size; i++) {

			scf_3ac_operand_t* src = c->srcs->data[i];
			if (!src || !src->node)
				continue;

			ret = scf_dag_get_node(dag, src->node, &src->dag_node);
			if (ret < 0)
				return ret;

			if (!dag_dst)
				continue;

			int nb_operands = op->nb_operands;
			if (SCF_OP_ARRAY_INDEX == op->type)
				nb_operands = 3;
			else if (SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX == c->op->type)
				nb_operands = 3;
			else if (SCF_OP_3AC_ADDRESS_OF_POINTER == c->op->type)
				nb_operands = 2;

			if (!dag_dst->childs || dag_dst->childs->size < nb_operands) {

				ret = scf_dag_node_add_child(dag_dst, src->dag_node);
				if (ret < 0)
					return ret;
			} else {
				int j;
				for (j = 0; j < dag_dst->childs->size; j++) {
					if (src->dag_node == dag_dst->childs->data[j])
						break;
				}
				if (j == dag_dst->childs->size) {
					scf_loge("c->op: %s, op: %s\n", c->op->name, op->name);
					return -1;
				}
			}
		}
	}

	return 0;
}

static void _3ac_find_basic_block_start(scf_list_t* h)
{
	int			  start = 0;
	scf_list_t*	  l;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		scf_list_t*		l2 = NULL;
		scf_3ac_code_t*	c2 = NULL;

		if (!start) {
			c->basic_block_start = 1;
			start = 1;
		}
#if 0
		if (scf_type_is_assign_dereference(c->op->type)) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				c2 = scf_list_data(l2, scf_3ac_code_t, list);
				c2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
			continue;
		}
		if (SCF_OP_DEREFERENCE == c->op->type) {
			c->basic_block_start = 1;
			continue;
		}
#endif

		if (SCF_OP_CALL == c->op->type
				|| SCF_OP_3AC_CALL_EXTERN == c->op->type) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				c2 = scf_list_data(l2, scf_3ac_code_t, list);
				c2->basic_block_start = 1;
			}

//			c->basic_block_start = 1;
			continue;
		}

		if (SCF_OP_RETURN == c->op->type
				|| SCF_OP_3AC_END == c->op->type) {
			c->basic_block_start = 1;
			continue;
		}

		if (SCF_OP_3AC_CMP == c->op->type
				|| SCF_OP_3AC_TEQ == c->op->type) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {

				c2 = scf_list_data(l2, scf_3ac_code_t, list);

				if (scf_type_is_jmp(c2->op->type))
					c->basic_block_start = 1;
			}
			continue;
		}

		if (scf_type_is_jmp(c->op->type)) {
			assert(c->dst->code);

			for (l2 = &c->dst->code->list; l2 != scf_list_sentinel(h); ) {

				c2 = scf_list_data(l2, scf_3ac_code_t, list);

				if (SCF_OP_GOTO == c2->op->type) {
					l2 = &c2->dst->code->list;
					continue;
				}

				if (SCF_OP_3AC_NOP == c2->op->type) {
					l2 = scf_list_next(l2);
					continue;
				}

				if (!scf_type_is_jmp(c2->op->type)) {
					c->dst->code          = c2;
					c2->basic_block_start = 1;
					c2->jmp_dst_flag      = 1;
					break;
				}

				if (SCF_OP_GOTO == c->op->type) {
					c->op        = c2->op;
					c->dst->code = c2->dst->code;

					l2 = &c2->dst->code->list;
					continue;
				}

				scf_logw("c: %s, c2: %s\n", c->op->name, c2->op->name);
				c->dst->code = c2;
				c2->basic_block_start = 1;
				c2->jmp_dst_flag      = 1;
				break;
			}

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				c2 = scf_list_data(l2, scf_3ac_code_t, list);
				c2->basic_block_start = 1;
			}

			c->basic_block_start = 1;
		}
	}
#if 1
	for (l = scf_list_head(h); l != scf_list_sentinel(h); ) {

		scf_3ac_code_t* c  = scf_list_data(l, scf_3ac_code_t, list);

		scf_list_t*		l2 = NULL;
		scf_3ac_code_t*	c2 = NULL;

		if (SCF_OP_3AC_NOP == c->op->type) {
			assert(!c->jmp_dst_flag);

			l = scf_list_next(l);

			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;
			continue;
		}

		if (SCF_OP_GOTO != c->op->type) {
			l = scf_list_next(l);
			continue;
		}
		assert(!c->jmp_dst_flag);

		for (l2 = scf_list_next(&c->list); l2 != scf_list_sentinel(h); ) {

			c2  = scf_list_data(l2, scf_3ac_code_t, list);

			if (c2->jmp_dst_flag)
				break;

			l2 = scf_list_next(l2);

			scf_list_del(&c2->list);
			scf_3ac_code_free(c2);
			c2 = NULL;
		}

		l = scf_list_next(l);

		if (l == &c->dst->code->list) {
			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;
		}
	}
#endif
}

static int _3ac_split_basic_blocks(scf_list_t* h, scf_function_t* f)
{
	scf_list_t*	l;
	scf_basic_block_t* bb = NULL;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); ) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		l = scf_list_next(l);

		if (c->basic_block_start) {

			bb = scf_basic_block_alloc();
			if (!bb)
				return -ENOMEM;

			bb->index = f->nb_basic_blocks++;
			scf_list_add_tail(&f->basic_block_list_head, &bb->list);

			c->basic_block = bb;

			if (SCF_OP_3AC_CMP == c->op->type
					|| SCF_OP_3AC_TEQ == c->op->type) {

				scf_list_t*	    l2;
				scf_3ac_code_t* c2;

				l2	= scf_list_next(&c->list);
				if (l2 != scf_list_sentinel(h)) {

					c2 = scf_list_data(l2, scf_3ac_code_t, list);

					if (scf_type_is_jmp(c2->op->type))
						bb->cmp_flag = 1;
				}

				scf_list_del(&c->list);
				scf_list_add_tail(&bb->code_list_head, &c->list);
				continue;
			}

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);

			if (SCF_OP_CALL == c->op->type || SCF_OP_3AC_CALL_EXTERN == c->op->type) {
				bb->call_flag = 1;
				continue;
			}

			if (SCF_OP_RETURN == c->op->type) {
				bb->ret_flag = 1;
				continue;
			}

			if (SCF_OP_3AC_END == c->op->type) {
				bb->end_flag = 1;
				continue;
			}

			if (scf_type_is_assign_dereference(c->op->type)
					|| SCF_OP_DEREFERENCE == c->op->type) {
				bb->dereference_flag = 1;
				continue;
			}

			if (scf_type_is_jmp(c->op->type)) {
				bb->jmp_flag = 1;

				if (scf_type_is_jcc(c->op->type))
					bb->jcc_flag = 1;

				int ret = scf_vector_add_unique(f->jmps, c);
				if (ret < 0)
					return ret;
			}
		} else {
			assert(bb);
			c->basic_block = bb;

			if (scf_type_is_assign_dereference(c->op->type)
					|| SCF_OP_DEREFERENCE == c->op->type)
				bb->dereference_flag = 1;

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);
		}
	}

	return 0;
}

static int _3ac_connect_basic_blocks(scf_function_t* f)
{
	int i;
	int ret;

	scf_list_t*	l;
	scf_list_t* sentinel = scf_list_sentinel(&f->basic_block_list_head);

	for (l = scf_list_head(&f->basic_block_list_head); l != sentinel; l = scf_list_next(l)) {

		scf_basic_block_t* current_bb = scf_list_data(l, scf_basic_block_t, list);
		scf_basic_block_t* prev_bb    = NULL;
		scf_basic_block_t* next_bb    = NULL;
		scf_list_t*        l2         = scf_list_prev(l);

		if (current_bb->jmp_flag)
			continue;

		if (l2 != sentinel) {
			prev_bb = scf_list_data(l2, scf_basic_block_t, list);

			if (!prev_bb->jmp_flag) {
				ret = scf_basic_block_connect(prev_bb, current_bb);
				if (ret < 0)
					return ret;
			}
		}

		if (current_bb->jmp_flag && !current_bb->jcc_flag)
			continue;

		l2 = scf_list_next(l);
		if (l2 == sentinel)
			continue;

		next_bb = scf_list_data(l2, scf_basic_block_t, list);

		if (!next_bb->jmp_flag) {
			ret = scf_basic_block_connect(current_bb, next_bb);
			if (ret < 0)
				return ret;
		}
	}

	for (i = 0; i < f->jmps->size; i++) {
		scf_3ac_code_t* c   = f->jmps->data[i];
		scf_3ac_code_t* dst = c->dst->code;

		scf_basic_block_t* current_bb = c->basic_block;
		scf_basic_block_t* dst_bb     = dst->basic_block;
		scf_basic_block_t* prev_bb    = NULL;
		scf_basic_block_t* next_bb    = NULL;

		c->dst->bb   = dst_bb;
		c->dst->code = NULL;

		l = scf_list_prev(&current_bb->list);
		if (l == sentinel)
			continue;

		prev_bb = scf_list_data(l, scf_basic_block_t, list);

		ret = scf_basic_block_connect(prev_bb, dst_bb);
		if (ret < 0)
			return ret;

		l = scf_list_next(&current_bb->list);
		if (l != sentinel && current_bb->jcc_flag) {

			next_bb = scf_list_data(l, scf_basic_block_t, list);

			ret = scf_basic_block_connect(prev_bb, next_bb);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

int scf_3ac_split_basic_blocks(scf_list_t* list_head_3ac, scf_function_t* f)
{
	_3ac_find_basic_block_start(list_head_3ac);

	int ret = _3ac_split_basic_blocks(list_head_3ac, f);
	if (ret < 0)
		return ret;

	return _3ac_connect_basic_blocks(f);
}

