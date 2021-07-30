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

	{SCF_OP_VA_START,       "va_start"},
	{SCF_OP_VA_ARG,         "va_arg"},
	{SCF_OP_VA_END,         "va_end"},

	{SCF_OP_RETURN,			 "return"},
	{SCF_OP_GOTO,			 "jmp"},

	{SCF_OP_3AC_TEQ,         "teq"},
	{SCF_OP_3AC_CMP,         "cmp"},

	{SCF_OP_3AC_LEA,         "lea"},

	{SCF_OP_3AC_SETZ,        "setz"},
	{SCF_OP_3AC_SETNZ,       "setnz"},
	{SCF_OP_3AC_SETGT,       "setgt"},
	{SCF_OP_3AC_SETGE,       "setge"},
	{SCF_OP_3AC_SETLT,       "setlt"},
	{SCF_OP_3AC_SETLE,       "setle"},

	{SCF_OP_3AC_SETA,        "seta"},
	{SCF_OP_3AC_SETAE,       "setae"},
	{SCF_OP_3AC_SETB,        "setb"},
	{SCF_OP_3AC_SETBE,       "setbe"},

	{SCF_OP_3AC_JZ,          "jz"},
	{SCF_OP_3AC_JNZ,         "jnz"},
	{SCF_OP_3AC_JGT,         "jgt"},
	{SCF_OP_3AC_JGE,         "jge"},
	{SCF_OP_3AC_JLT,         "jlt"},
	{SCF_OP_3AC_JLE,         "jle"},

	{SCF_OP_3AC_JA,          "ja"},
	{SCF_OP_3AC_JAE,         "jae"},
	{SCF_OP_3AC_JB,          "jb"},
	{SCF_OP_3AC_JBE,         "jbe"},

	{SCF_OP_3AC_NOP,         "nop"},
	{SCF_OP_3AC_END,         "end"},

	{SCF_OP_3AC_PUSH,        "push"},
	{SCF_OP_3AC_POP,         "pop"},
	{SCF_OP_3AC_SAVE,        "save"},
	{SCF_OP_3AC_LOAD,        "load"},
	{SCF_OP_3AC_RELOAD,      "reload"},
	{SCF_OP_3AC_RESAVE,      "resave"},

	{SCF_OP_3AC_PUSH_RAX,    "push rax"},
	{SCF_OP_3AC_POP_RAX,     "pop  rax"},
	{SCF_OP_3AC_MEMSET,      "memset"},

	{SCF_OP_3AC_INC,         "inc3"},
	{SCF_OP_3AC_DEC,         "dec3"},

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

	if (c0->dsts) {
		if (!c1->dsts)
			return 0;

		if (c0->dsts->size != c1->dsts->size)
			return 0;

		int i;
		for (i = 0; i < c0->dsts->size; i++) {
			scf_3ac_operand_t* dst0 = c0->dsts->data[i];
			scf_3ac_operand_t* dst1 = c1->dsts->data[i];

			if (dst0->dag_node) {
				if (!dst1->dag_node)
					return 0;

				if (!scf_dag_dn_same(dst0->dag_node, dst1->dag_node))
					return 0;
			} else if (dst1->dag_node)
				return 0;
		}
	} else if (c1->dsts)
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

	if (c->dsts) {
		c2->dsts = scf_vector_alloc();
		if (!c2->dsts) {
			scf_3ac_code_free(c2);
			return NULL;
		}

		int i;
		for (i = 0; i < c->dsts->size; i++) {
			scf_3ac_operand_t* dst  = c->dsts->data[i];
			scf_3ac_operand_t* dst2 = scf_3ac_operand_alloc();

			if (!dst2) {
				scf_3ac_code_free(c2);
				return NULL;
			}

			if (scf_vector_add(c2->dsts, dst2) < 0) {
				scf_3ac_code_free(c2);
				return NULL;
			}

			dst2->node     = dst->node;
			dst2->dag_node = dst->dag_node;
			dst2->code     = dst->code;
			dst2->bb       = dst->bb;
			dst2->debug_w  = dst->debug_w;
		}
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
			src2->bb       = src->bb;
			src2->debug_w  = src->debug_w;
		}
	}

	c2->label = c->label;
	c2->error = c->error;

	c2->origin = c;
	return c2;
}

void scf_3ac_code_free(scf_3ac_code_t* c)
{
	int i;

	if (c) {
		if (c->dsts) {
			for (i = 0; i < c->dsts->size; i++)
				scf_3ac_operand_free(c->dsts->data[i]);
			scf_vector_free(c->dsts);
		}

		if (c->srcs) {
			for (i = 0; i < c->srcs->size; i++)
				scf_3ac_operand_free(c->srcs->data[i]);
			scf_vector_free(c->srcs);
		}

		if (c->active_vars) {
			int i;
			for (i = 0; i < c->active_vars->size; i++)
				scf_dn_status_free(c->active_vars->data[i]);
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
	scf_3ac_operator_t* op;
	scf_3ac_operand_t*  d;
	scf_3ac_code_t*     c;

	op = scf_3ac_find_operator(op_type);
	if (!op) {
		scf_loge("3ac operator not found\n");
		return NULL;
	}

	d = scf_3ac_operand_alloc();
	if (!d)
		return NULL;
	d->dag_node = dst;

	c = scf_3ac_code_alloc();
	if (!c) {
		scf_3ac_operand_free(d);
		return NULL;
	}

	c->dsts = scf_vector_alloc();
	if (!c->dsts) {
		scf_3ac_operand_free(d);
		scf_3ac_code_free(c);
		return NULL;
	}

	if (scf_vector_add(c->dsts, d) < 0) {
		scf_3ac_operand_free(d);
		scf_3ac_code_free(c);
		return NULL;
	}

	c->op = op;
	return c;
}

scf_3ac_code_t* scf_branch_ops_code(int type, scf_label_t* l, scf_node_t* err)
{
	scf_3ac_operand_t* dst;
	scf_3ac_code_t*    c;

	c = scf_3ac_code_alloc();
	if (!c)
		return NULL;

	c->op    = scf_3ac_find_operator(type);
	c->label = l;
	c->error = err;

	c->dsts  = scf_vector_alloc();
	if (!c->dsts) {
		scf_3ac_code_free(c);
		return NULL;
	}

	dst = scf_3ac_operand_alloc();
	if (!dst) {
		scf_3ac_code_free(c);
		return NULL;
	}

	if (scf_vector_add(c->dsts, dst) < 0) {

		scf_3ac_operand_free(dst);
		scf_3ac_code_free(c);
		return NULL;
	}

	return c;
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
			printf("v_%d_%d/%s_%#lx ",
					dn->var->w->line, dn->var->w->pos, dn->var->w->text->data,
					0xffff & (uintptr_t)dn);
		} else {
			printf("v_%#lx", 0xffff & (uintptr_t)dn->var);
		}
	} else if (scf_type_is_operator(dn->type)) {
		scf_3ac_operator_t* op = scf_3ac_find_operator(dn->type);
		if (dn->var && dn->var->w)
			printf("v_%d_%d/%s_%#lx ",
					dn->var->w->line, dn->var->w->pos, dn->var->w->text->data,
					0xffff & (uintptr_t)dn);
		else
			printf("v_%#lx/%s ", 0xffff & (uintptr_t)dn->var, op->name);
	} else {
		//printf("type: %d, v_%#lx\n", dn->type, 0xffff & (uintptr_t)dn->var);
		//assert(0);
	}
}

void scf_3ac_code_print(scf_3ac_code_t* c, scf_list_t* sentinel)
{
	scf_3ac_operand_t* src;
	scf_3ac_operand_t* dst;

	int i;

	printf("%s  ", c->op->name);

	if (c->dsts) {

		for (i  = 0; i < c->dsts->size; i++) {
			dst =        c->dsts->data[i];

			if (dst->dag_node)
				_3ac_print_dag_node(dst->dag_node);

			else if (dst->node)
				_3ac_print_node(dst->node);

			else if (dst->code) {
				if (&dst->code->list != sentinel) {
					printf(": ");
					scf_3ac_code_print(dst->code, sentinel);
				}
			} else if (dst->bb)
				printf(" bb: %p, index: %d ", dst->bb, dst->bb->index);

			if (i < c->dsts->size - 1)
				printf(", ");
		}
	}

	if (c->srcs) {
	
		for (i  = 0; i < c->srcs->size; i++) {
			src =        c->srcs->data[i];

			if (0 == i && c->dsts && c->dsts->size > 0)
				printf("; ");

			if (src->dag_node)
				_3ac_print_dag_node(src->dag_node);

			else if (src->node)
				_3ac_print_node(src->node);

			else if (src->code)
				assert(0);

			if (i < c->srcs->size - 1)
				printf(", ");
		}
	}

	printf("\n");
}

static int _3ac_code_to_dag(scf_3ac_code_t* c, scf_list_t* dag, int nb_operands0, int nb_operands1)
{
	scf_3ac_operand_t* dst;
	scf_3ac_operand_t* src;
	scf_dag_node_t*    dn;

	int ret;
	int i;
	int j;

	if (c->dsts) {

		for (j  = 0; j < c->dsts->size; j++) {
			dst =        c->dsts->data[j];

			if (!dst || !dst->node)
				continue;

			ret = scf_dag_get_node(dag, dst->node, &dst->dag_node);
			if (ret < 0)
				return ret;
		}
	}

	if (!c->srcs)
		return 0;

	for (i  = 0; i < c->srcs->size; i++) {
		src =        c->srcs->data[i];

		if (!src || !src->node)
			continue;

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		if (!c->dsts)
			continue;

		for (j  = 0; j < c->dsts->size; j++) {
			dst =        c->dsts->data[j];

			if (!dst || !dst->dag_node)
				continue;

			dn = dst->dag_node;

			if (!dn->childs || dn->childs->size < nb_operands0) {

				ret = scf_dag_node_add_child(dn, src->dag_node);
				if (ret < 0)
					return ret;
				continue;
			}

			int k;
			for (k = 0; k < dn->childs->size && k < nb_operands1; k++) {

				if (src->dag_node == dn->childs->data[k])
					break;
			}

			if (k == dn->childs->size) {
				scf_loge("i: %d, c->op: %s, dn->childs->size: %d, c->srcs->size: %d\n",
						i, c->op->name, dn->childs->size, c->srcs->size);
				scf_3ac_code_print(c, NULL);
				return -1;
			}
		}
	}

	return 0;
}

int scf_3ac_code_to_dag(scf_3ac_code_t* c, scf_list_t* dag)
{
	int ret = 0;

	if (scf_type_is_assign(c->op->type)) {

		scf_3ac_operand_t* src = c->srcs->data[0];
		scf_3ac_operand_t* dst = c->dsts->data[0];

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_get_node(dag, dst->node, &dst->dag_node);
		if (ret < 0)
			return ret;

		scf_dag_node_t* dn_src;
		scf_dag_node_t* dn_parent;
		scf_dag_node_t* dn_child;
		scf_dag_node_t* dn_assign;
		scf_variable_t* v_assign = NULL;

		if (dst->node->parent)
			v_assign = _scf_operand_get(dst->node->parent);

		dn_assign = scf_dag_node_alloc(c->op->type, v_assign, NULL);

		scf_list_add_tail(dag, &dn_assign->list);

		ret = scf_dag_node_add_child(dn_assign, dst->dag_node);
		if (ret < 0)
			return ret;

		dn_src = src->dag_node;

		if (dn_src->parents && dn_src->parents->size > 0) {
			dn_parent        = dn_src->parents->data[dn_src->parents->size - 1];

			if (SCF_OP_ASSIGN == dn_parent->type) {
				assert(2      == dn_parent->childs->size);
				dn_child      =  dn_parent->childs->data[1];

				return scf_dag_node_add_child(dn_assign, dn_child);
			}
		}

		ret = scf_dag_node_add_child(dn_assign, src->dag_node);
		if (ret < 0)
			return ret;

	} else if (scf_type_is_assign_array_index(c->op->type)
			|| scf_type_is_assign_pointer(c->op->type)) {

		scf_3ac_operand_t* src;
		scf_dag_node_t*    assign;

		assert(c->srcs);

		assign = scf_dag_node_alloc(c->op->type, NULL, NULL);
		if (!assign)
			return -ENOMEM;
		scf_list_add_tail(dag, &assign->list);

		int i;
		for (i  = 0; i < c->srcs->size; i++) {
			src = c->srcs->data[i];
			assert(src && src->node);

			ret = scf_dag_get_node(dag, src->node, &src->dag_node);
			if (ret < 0)
				return ret;

			ret = scf_dag_node_add_child(assign, src->dag_node);
			if (ret < 0)
				return ret;
		}
	} else if (SCF_OP_3AC_CMP == c->op->type
			|| SCF_OP_3AC_TEQ == c->op->type) {

		scf_3ac_operand_t* src;
		scf_dag_node_t*    dn_cmp = scf_dag_node_alloc(c->op->type, NULL, NULL);

		scf_list_add_tail(dag, &dn_cmp->list);

		if (c->srcs) {
			int i;
			for (i  = 0; i < c->srcs->size; i++) {
				src = c->srcs->data[i];

				ret = scf_dag_get_node(dag, src->node, &src->dag_node);
				if (ret < 0)
					return ret;

				ret = scf_dag_node_add_child(dn_cmp, src->dag_node);
				if (ret < 0)
					return ret;
			}
		}
	} else if (SCF_OP_3AC_SETZ  == c->op->type
			|| SCF_OP_3AC_SETNZ == c->op->type
			|| SCF_OP_3AC_SETLT == c->op->type
			|| SCF_OP_3AC_SETLE == c->op->type
			|| SCF_OP_3AC_SETGT == c->op->type
			|| SCF_OP_3AC_SETGE == c->op->type) {

		assert(c->dsts && 1 == c->dsts->size);

		scf_3ac_operand_t* dst      = c->dsts->data[0];

		scf_dag_node_t*    dn_setcc = scf_dag_node_alloc(c->op->type, NULL, NULL);
		scf_list_add_tail(dag, &dn_setcc->list);

		ret = scf_dag_get_node(dag, dst->node, &dst->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_node_add_child(dn_setcc, dst->dag_node);
		if (ret < 0)
			return ret;

	} else if (SCF_OP_INC == c->op->type
			|| SCF_OP_DEC == c->op->type
			|| SCF_OP_INC_POST == c->op->type
			|| SCF_OP_DEC_POST == c->op->type
			|| SCF_OP_3AC_INC  == c->op->type
			|| SCF_OP_3AC_DEC  == c->op->type) {

		scf_3ac_operand_t* src = c->srcs->data[0];

		assert(src->node->parent);
		scf_variable_t*    var_parent = _scf_operand_get(src->node->parent);
		scf_dag_node_t*    dn_parent  = scf_dag_node_alloc(c->op->type, var_parent, NULL);
		scf_list_add_tail(dag, &dn_parent->list);

		ret = scf_dag_get_node(dag, src->node, &src->dag_node);
		if (ret < 0)
			return ret;

		ret = scf_dag_node_add_child(dn_parent, src->dag_node);
		if (ret < 0)
			return ret;

		if (c->dsts) {
			scf_3ac_operand_t* dst = c->dsts->data[0];
			assert(dst->node);
			dst->dag_node = dn_parent;
		}
	} else if (SCF_OP_RETURN == c->op->type) {

		if (c->srcs) {
			scf_3ac_operand_t* src;

			int i;
			for (i  = 0; i < c->srcs->size; i++) {
				src =        c->srcs->data[i];

				ret = scf_dag_get_node(dag, src->node, &src->dag_node);
				if (ret < 0)
					return ret;
			}
		}
	} else if (scf_type_is_jmp(c->op->type)) {

		scf_logd("c->op: %d, name: %s\n", c->op->type, c->op->name);

	} else {
		int nb_operands0   = -1;
		int nb_operands1   = -1;

		if (c->dsts) {
			scf_3ac_operand_t* dst0 = c->dsts->data[0];

			assert(dst0->node->op);

			nb_operands0 = dst0->node->op->nb_operands;
			nb_operands1 = nb_operands0;

			switch (c->op->type) {
				case SCF_OP_ARRAY_INDEX:
				case SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX:
				case SCF_OP_3AC_INC_POST_ARRAY_INDEX:
				case SCF_OP_3AC_DEC_POST_ARRAY_INDEX:
				case SCF_OP_VA_START:
				case SCF_OP_VA_ARG:
					nb_operands0 = 3;
					break;

				case SCF_OP_3AC_ADDRESS_OF_POINTER:
				case SCF_OP_VA_END:
					nb_operands0 = 2;
					break;

				case SCF_OP_CALL:
					nb_operands0 = c->srcs->size;
					break;
				default:
					break;
			};
		}

		return _3ac_code_to_dag(c, dag, nb_operands0, nb_operands1);
	}

	return 0;
}

static void _3ac_filter_jmp(scf_list_t* h, scf_3ac_code_t* c)
{
	scf_list_t*        l2   = NULL;
	scf_3ac_code_t*    c2   = NULL;
	scf_3ac_operand_t* dst0 = c->dsts->data[0];
	scf_3ac_operand_t* dst1 = NULL;

	for (l2 = &dst0->code->list; l2 != scf_list_sentinel(h); ) {

		c2 = scf_list_data(l2, scf_3ac_code_t, list);

		if (SCF_OP_GOTO == c2->op->type) {
			dst0 = c2->dsts->data[0];
			l2   = &dst0->code->list;
			continue;
		}

		if (SCF_OP_3AC_NOP == c2->op->type) {
			l2 = scf_list_next(l2);
			continue;
		}

		if (!scf_type_is_jmp(c2->op->type)) {

			dst0       = c->dsts->data[0];
			dst0->code = c2;

			c2->basic_block_start = 1;
			c2->jmp_dst_flag      = 1;
			break;
		}

		if (SCF_OP_GOTO == c->op->type) {
			c->op        = c2->op;

			dst0 = c ->dsts->data[0];
			dst1 = c2->dsts->data[0];

			dst0->code = dst1->code;

			l2 = &dst1->code->list;
			continue;
		}

		scf_logw("c: %s, c2: %s\n", c->op->name, c2->op->name);
		dst0       = c->dsts->data[0];
		dst0->code = c2;
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

static void _3ac_filter_dst_teq(scf_list_t* h, scf_3ac_code_t* c)
{
	scf_3ac_code_t*	   setcc = NULL;
	scf_3ac_code_t*	   jcc   = NULL;
	scf_3ac_operand_t* dst0  = c->dsts->data[0];
	scf_3ac_operand_t* dst1  = NULL;
	scf_3ac_code_t*    teq   = dst0->code;
	scf_list_t*        l;

	if (scf_list_prev(&c->list) == scf_list_sentinel(h))
		return;

	if (scf_list_next(&teq->list) == scf_list_sentinel(h))
		return;

	setcc = scf_list_data(scf_list_prev(&c->list),   scf_3ac_code_t, list);
	jcc   = scf_list_data(scf_list_next(&teq->list), scf_3ac_code_t, list);

	if ((SCF_OP_3AC_JNZ == c->op->type && SCF_OP_3AC_SETZ == setcc->op->type)
			|| (SCF_OP_3AC_JZ  == c->op->type && SCF_OP_3AC_SETNZ == setcc->op->type)
			|| (SCF_OP_3AC_JGT == c->op->type && SCF_OP_3AC_SETLE == setcc->op->type)
			|| (SCF_OP_3AC_JGE == c->op->type && SCF_OP_3AC_SETLT == setcc->op->type)
			|| (SCF_OP_3AC_JLT == c->op->type && SCF_OP_3AC_SETGE == setcc->op->type)
			|| (SCF_OP_3AC_JLE == c->op->type && SCF_OP_3AC_SETGT == setcc->op->type)) {

		assert(setcc->dsts && 1 == setcc->dsts->size);
		assert(teq->srcs   && 1 == teq  ->srcs->size);

		scf_3ac_operand_t* src   = teq  ->srcs->data[0];
		scf_3ac_operand_t* dst   = setcc->dsts->data[0];
		scf_variable_t*    v_teq = _scf_operand_get(src->node);
		scf_variable_t*    v_set = _scf_operand_get(dst->node);

		if (v_teq != v_set)
			return;

		if (SCF_OP_3AC_JZ == jcc->op ->type) {

			dst0 = c  ->dsts->data[0];
			dst1 = jcc->dsts->data[0];

			dst0->code = dst1->code;

		} else if (SCF_OP_3AC_JNZ == jcc->op->type) {

			l = scf_list_next(&jcc->list);
			if (l == scf_list_sentinel(h))
				return;

			dst0       = c->dsts->data[0];
			dst0->code = scf_list_data(l, scf_3ac_code_t, list);
		}
	} else if ((SCF_OP_3AC_JNZ == c->op->type && SCF_OP_3AC_SETNZ == setcc->op->type)
			|| (SCF_OP_3AC_JZ  == c->op->type && SCF_OP_3AC_SETZ  == setcc->op->type)
			|| (SCF_OP_3AC_JGT == c->op->type && SCF_OP_3AC_SETGT == setcc->op->type)
			|| (SCF_OP_3AC_JGE == c->op->type && SCF_OP_3AC_SETGE == setcc->op->type)
			|| (SCF_OP_3AC_JLT == c->op->type && SCF_OP_3AC_SETLT == setcc->op->type)
			|| (SCF_OP_3AC_JLE == c->op->type && SCF_OP_3AC_SETLE == setcc->op->type)) {

		assert(setcc->dsts && 1 == setcc->dsts->size);
		assert(teq->srcs   && 1 == teq  ->srcs->size);

		scf_3ac_operand_t* src   = teq  ->srcs->data[0];
		scf_3ac_operand_t* dst   = setcc->dsts->data[0];
		scf_variable_t*    v_teq = _scf_operand_get(src->node);
		scf_variable_t*    v_set = _scf_operand_get(dst->node);

		if (v_teq != v_set)
			return;

		if (SCF_OP_3AC_JNZ == jcc-> op->type) {

			dst0 = c  ->dsts->data[0];
			dst1 = jcc->dsts->data[0];

			dst0->code = dst1->code;

		} else if (SCF_OP_3AC_JZ == jcc->op->type) {

			l = scf_list_next(&jcc->list);
			if (l == scf_list_sentinel(h))
				return;

			dst0       = c->dsts->data[0];
			dst0->code = scf_list_data(l, scf_3ac_code_t, list);
		}
	}
}

static int _3ac_filter_prev_teq(scf_list_t* h, scf_3ac_code_t* c, scf_3ac_code_t* teq)
{
	scf_3ac_code_t*	setcc = NULL;
	scf_3ac_code_t*	jcc   = NULL;
	scf_3ac_code_t*	jmp   = NULL;
	scf_list_t*     l;

	int jcc_type;

	if (scf_list_prev(&teq->list) == scf_list_sentinel(h))
		return 0;

	setcc    = scf_list_data(scf_list_prev(&teq->list), scf_3ac_code_t, list);
	jcc_type = -1;

	if (SCF_OP_3AC_JZ == c->op->type) {

		switch (setcc->op->type) {
			case SCF_OP_3AC_SETZ:
				jcc_type = SCF_OP_3AC_JNZ;
				break;

			case SCF_OP_3AC_SETNZ:
				jcc_type = SCF_OP_3AC_JZ;
				break;

			case SCF_OP_3AC_SETGT:
				jcc_type = SCF_OP_3AC_JLE;
				break;

			case SCF_OP_3AC_SETGE:
				jcc_type = SCF_OP_3AC_JLT;
				break;

			case SCF_OP_3AC_SETLT:
				jcc_type = SCF_OP_3AC_JGE;
				break;

			case SCF_OP_3AC_SETLE:
				jcc_type = SCF_OP_3AC_JGT;
				break;

			case SCF_OP_3AC_SETA:
				jcc_type = SCF_OP_3AC_JBE;
				break;
			case SCF_OP_3AC_SETAE:
				jcc_type = SCF_OP_3AC_JB;
				break;

			case SCF_OP_3AC_SETB:
				jcc_type = SCF_OP_3AC_JAE;
				break;
			case SCF_OP_3AC_SETBE:
				jcc_type = SCF_OP_3AC_JA;
				break;
			default:
				return 0;
				break;
		};
	} else if (SCF_OP_3AC_JNZ == c->op->type) {

		switch (setcc->op->type) {
			case SCF_OP_3AC_SETZ:
				jcc_type = SCF_OP_3AC_JZ;
				break;

			case SCF_OP_3AC_SETNZ:
				jcc_type = SCF_OP_3AC_JNZ;
				break;

			case SCF_OP_3AC_SETGT:
				jcc_type = SCF_OP_3AC_JGT;
				break;

			case SCF_OP_3AC_SETGE:
				jcc_type = SCF_OP_3AC_JGE;
				break;

			case SCF_OP_3AC_SETLT:
				jcc_type = SCF_OP_3AC_JLT;
				break;

			case SCF_OP_3AC_SETLE:
				jcc_type = SCF_OP_3AC_JLE;
				break;

			case SCF_OP_3AC_SETA:
				jcc_type = SCF_OP_3AC_JA;
				break;
			case SCF_OP_3AC_SETAE:
				jcc_type = SCF_OP_3AC_JAE;
				break;

			case SCF_OP_3AC_SETB:
				jcc_type = SCF_OP_3AC_JB;
				break;
			case SCF_OP_3AC_SETBE:
				jcc_type = SCF_OP_3AC_JBE;
				break;
			default:
				return 0;
				break;
		};
	} else
		return 0;

	assert(setcc->dsts && 1 == setcc->dsts->size);
	assert(teq->srcs   && 1 == teq  ->srcs->size);

	scf_3ac_operand_t* src   = teq  ->srcs->data[0];
	scf_3ac_operand_t* dst0  = setcc->dsts->data[0];
	scf_3ac_operand_t* dst1  = NULL;
	scf_variable_t*    v_teq = _scf_operand_get(src ->node);
	scf_variable_t*    v_set = _scf_operand_get(dst0->node);

	if (v_teq != v_set)
		return 0;

#define SCF_3AC_JCC_ALLOC(j, cc) \
	do { \
		j = scf_3ac_code_alloc(); \
		if (!j) \
			return -ENOMEM; \
		\
		j->dsts = scf_vector_alloc(); \
		if (!j->dsts) { \
			scf_3ac_code_free(j); \
			return -ENOMEM; \
		} \
		\
		scf_3ac_operand_t* dst0 = scf_3ac_operand_alloc(); \
		if (!dst0) { \
			scf_3ac_code_free(j); \
			return -ENOMEM; \
		} \
		\
		if (scf_vector_add(j->dsts, dst0) < 0) { \
			scf_3ac_code_free(j); \
			scf_3ac_operand_free(dst0); \
			return -ENOMEM; \
		} \
		j->op = scf_3ac_find_operator(cc); \
		assert(j->op); \
	} while (0)

	SCF_3AC_JCC_ALLOC(jcc, jcc_type);
	dst0 = jcc->dsts->data[0];
	dst1 = c  ->dsts->data[0];
	dst0->code = dst1->code;
	scf_list_add_front(&setcc->list, &jcc->list);

	l = scf_list_next(&c->list);
	if (l == scf_list_sentinel(h)) {
		_3ac_filter_jmp(h, jcc);
		return 0;
	}
#if 1
	SCF_3AC_JCC_ALLOC(jmp, SCF_OP_GOTO);
	dst0 = jmp->dsts->data[0];
	dst0->code = scf_list_data(l, scf_3ac_code_t, list);
	scf_list_add_front(&jcc->list, &jmp->list);
#endif

	_3ac_filter_jmp(h, jcc);
	_3ac_filter_jmp(h, jmp);

	return 0;
}

static int _3ac_find_basic_block_start(scf_list_t* h)
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

#if 0
		if (SCF_OP_CALL == c->op->type) {

			l2	= scf_list_next(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				c2 = scf_list_data(l2, scf_3ac_code_t, list);
				c2->basic_block_start = 1;
			}

//			c->basic_block_start = 1;
			continue;
		}
#endif

#if 0
		if (SCF_OP_RETURN == c->op->type) {
			c->basic_block_start = 1;
			continue;
		}
#endif

		if (SCF_OP_3AC_END == c->op->type) {
			c->basic_block_start = 1;
			continue;
		}

		if (SCF_OP_3AC_CMP == c->op->type
				|| SCF_OP_3AC_TEQ == c->op->type) {

			for (l2 = scf_list_next(&c->list); l2 != scf_list_sentinel(h); l2 = scf_list_next(l2)) {

				c2  = scf_list_data(l2, scf_3ac_code_t, list);

				if (scf_type_is_setcc(c2->op->type))
					continue;

				if (scf_type_is_jmp(c2->op->type))
					c->basic_block_start = 1;
				break;
			}
			continue;
		}

		if (scf_type_is_jmp(c->op->type)) {

			scf_3ac_operand_t* dst0 = c->dsts->data[0];

			assert(dst0->code);

			// filter 1st expr of logic op, such as '&&', '||'
			if (SCF_OP_3AC_TEQ == dst0->code->op->type)
				_3ac_filter_dst_teq(h, c);

			l2 = scf_list_prev(&c->list);
			if (l2 != scf_list_sentinel(h)) {
				c2  = scf_list_data(l2, scf_3ac_code_t, list);

				// filter 2nd expr of logic op, such as '&&', '||'
				if (SCF_OP_3AC_TEQ == c2->op->type) {

					int ret = _3ac_filter_prev_teq(h, c, c2);
					if (ret < 0)
						return ret;
				}
			}

			_3ac_filter_jmp(h, c);
		}
	}
#if 1
	for (l = scf_list_head(h); l != scf_list_sentinel(h); ) {

		scf_3ac_code_t*    c    = scf_list_data(l, scf_3ac_code_t, list);

		scf_list_t*        l2   = NULL;
		scf_3ac_code_t*    c2   = NULL;
		scf_3ac_operand_t* dst0 = NULL;

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

		l    = scf_list_next(l);
		dst0 = c->dsts->data[0];

		if (l == &dst0->code->list) {
			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;
		}
	}
#endif
	return 0;
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

				for (l2 = scf_list_next(&c->list); l2 != scf_list_sentinel(h); l2 = scf_list_next(l2)) {

					c2  = scf_list_data(l2, scf_3ac_code_t, list);

					if (scf_type_is_setcc(c2->op->type))
						continue;

					if (scf_type_is_jmp(c2->op->type))
						bb->cmp_flag = 1;
					break;
				}

				scf_list_del(&c->list);
				scf_list_add_tail(&bb->code_list_head, &c->list);
				continue;
			}

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);

			if (SCF_OP_CALL == c->op->type) {
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

			if (scf_type_is_assign_array_index(c->op->type)) {
				bb->array_index_flag = 1;
				continue;
			}

			if (SCF_OP_VA_START      == c->op->type
					|| SCF_OP_VA_ARG == c->op->type
					|| SCF_OP_VA_END == c->op->type) {
				bb->varg_flag = 1;
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

			if (scf_type_is_assign_dereference(c->op->type) || SCF_OP_DEREFERENCE == c->op->type)
				bb->dereference_flag = 1;

			else if (scf_type_is_assign_array_index(c->op->type))
				bb->array_index_flag = 1;

			else if (SCF_OP_CALL == c->op->type)
				bb->call_flag = 1;

			else if (SCF_OP_RETURN == c->op->type)
				bb->ret_flag = 1;

			else if (SCF_OP_VA_START == c->op->type
					|| SCF_OP_VA_ARG == c->op->type
					|| SCF_OP_VA_END == c->op->type)
				bb->varg_flag = 1;

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
		scf_3ac_code_t*    c    = f->jmps->data[i];
		scf_3ac_operand_t* dst0 = c->dsts->data[0];
		scf_3ac_code_t*    dst  = dst0->code;

		scf_basic_block_t* current_bb = c->basic_block;
		scf_basic_block_t* dst_bb     = dst->basic_block;
		scf_basic_block_t* prev_bb    = NULL;
		scf_basic_block_t* next_bb    = NULL;

		dst0->bb   = dst_bb;
		dst0->code = NULL;

		for (l = scf_list_prev(&current_bb->list); l != sentinel; l = scf_list_prev(l)) {

			prev_bb = scf_list_data(l, scf_basic_block_t, list);

			if (!prev_bb->jmp_flag)
				break;

			if (!prev_bb->jcc_flag) {
				prev_bb = NULL;
				break;
			}
			prev_bb = NULL;
		}

		if (prev_bb) {
			ret = scf_basic_block_connect(prev_bb, dst_bb);
			if (ret < 0)
				return ret;
		} else
			continue;

		if (!current_bb->jcc_flag)
			continue;

		for (l = scf_list_next(&current_bb->list); l != sentinel; l = scf_list_next(l)) {

			next_bb = scf_list_data(l, scf_basic_block_t, list);

			if (!next_bb->jmp_flag)
				break;

			if (!next_bb->jcc_flag) {
				next_bb = NULL;
				break;
			}
			next_bb = NULL;
		}

		if (next_bb) {
			ret = scf_basic_block_connect(prev_bb, next_bb);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

int scf_3ac_split_basic_blocks(scf_list_t* list_head_3ac, scf_function_t* f)
{
	int ret = _3ac_find_basic_block_start(list_head_3ac);
	if (ret < 0)
		return ret;

	ret = _3ac_split_basic_blocks(list_head_3ac, f);
	if (ret < 0)
		return ret;

	return _3ac_connect_basic_blocks(f);
}

scf_3ac_code_t* scf_3ac_code_NN(int op_type, scf_node_t** dsts, int nb_dsts, scf_node_t** srcs, int nb_srcs)
{
	scf_3ac_operator_t* op = scf_3ac_find_operator(op_type);
	if (!op) {
		scf_loge("\n");
		return NULL;
	}

	scf_3ac_operand_t* operand;
	scf_3ac_code_t*    c;
	scf_vector_t*      vsrc = NULL;
	scf_vector_t*      vdst = NULL;
	scf_node_t*        node;

	int i;

	if (srcs) {
		vsrc = scf_vector_alloc();
		for (i = 0; i < nb_srcs; i++) {

			operand = scf_3ac_operand_alloc();

			node    = srcs[i];

			while (node && SCF_OP_EXPR == node->type)
				node = node->nodes[0];

			operand->node = node;
			if (node)
				operand->debug_w = node->debug_w;
			else
				operand->debug_w = NULL;

			scf_vector_add(vsrc, operand);
		}
	}

	if (dsts) {
		vdst = scf_vector_alloc();
		for (i = 0; i < nb_dsts; i++) {

			operand = scf_3ac_operand_alloc();

			node    = dsts[i];

			while (node && SCF_OP_EXPR == node->type)
				node = node->nodes[0];

			operand->node = node;
			if (node)
				operand->debug_w = node->debug_w;
			else
				operand->debug_w = NULL;

			scf_vector_add(vdst, operand);
		}
	}

	c       = scf_3ac_code_alloc();
	c->op   = op;
	c->dsts = vdst;
	c->srcs = vsrc;
	return c;
}
