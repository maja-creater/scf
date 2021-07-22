#include"scf_optimizer.h"
#include"scf_pointer_alias.h"

static int _3ac_pointer_alias(scf_dag_node_t* alias, scf_3ac_code_t* c, scf_basic_block_t* bb)
{
	scf_3ac_operand_t* pointer;

	int ret;

	assert(c->srcs && c->srcs->size >= 1);

	pointer = c->srcs->data[0];

	ret     = scf_vector_del(c->srcs, pointer);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

#if 1
	scf_3ac_code_t* c2;
	scf_3ac_code_t* c3;
	scf_list_t*     l2;
	scf_list_t*     l3;
	scf_list_t      h;

	scf_list_init(&h);

	ret = scf_dag_expr_calculate(&h, pointer->dag_node);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l2 = scf_list_head(&h); l2 != scf_list_sentinel(&h); ) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);
		l2  = scf_list_next(l2);

		for (l3 = scf_list_prev(&c->list); l3 != scf_list_sentinel(&bb->code_list_head); ) {

			c3  = scf_list_data(l3, scf_3ac_code_t, list);
			l3  = scf_list_prev(l3);

			if (scf_3ac_code_same(c2, c3)) {
				scf_list_del(&c3->list);
				scf_3ac_code_free(c3);
				break;
			}
		}

		scf_list_del(&c2->list);
		scf_3ac_code_free(c2);
	}
#endif

	pointer->dag_node = alias;

	if (!c->dsts) {
		c->dsts = scf_vector_alloc();
		if (!c->dsts)
			return -ENOMEM;

		if (scf_vector_add(c->dsts, pointer) < 0)
			return -ENOMEM;
	} else {
		SCF_XCHG(c->dsts->data[0], pointer);
		scf_3ac_operand_free(pointer);
	}
	pointer = NULL;

	switch (c->op->type) {
		case SCF_OP_3AC_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_ASSIGN);
			break;

		case SCF_OP_3AC_ADD_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_ADD_ASSIGN);
			break;
		case SCF_OP_3AC_SUB_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_SUB_ASSIGN);
			break;

		case SCF_OP_3AC_AND_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_AND_ASSIGN);
			break;
		case SCF_OP_3AC_OR_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_OR_ASSIGN);
			break;

		case SCF_OP_3AC_INC_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_INC);
			break;
		case SCF_OP_3AC_DEC_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_DEC);
			break;
		case SCF_OP_3AC_INC_POST_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_INC_POST);
			break;
		case SCF_OP_3AC_DEC_POST_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_DEC_POST);
			break;

		default:
			scf_loge("\n");
			return -1;
			break;
	};
	assert(c->op);
	return 0;
}

static int _alias_dereference(scf_vector_t** paliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_vector_t*      aliases;
	int ret;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __alias_dereference(aliases, dn_pointer, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		scf_vector_free(aliases);
		return ret;
	}

	*paliases = aliases;
	return 0;
}

static int _alias_assign_dereference(scf_vector_t** paliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* status;
	scf_vector_t*     aliases;
	int ret;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __alias_dereference(aliases, dn_pointer, c, bb, bb_list_head);
	if (ret < 0) {
		scf_vector_free(aliases);
		return ret;
	}

	scf_logd("aliases->size: %d\n", aliases->size);
	if (1 == aliases->size) {
		status = aliases->data[0];

		if (SCF_DN_ALIAS_VAR == status->alias_type) {
			ret = _3ac_pointer_alias(status->alias, c, bb);
			scf_vector_free(aliases);
			if (ret < 0)
				return ret;
			return scf_basic_block_inited_vars(bb, bb_list_head);
		}
		return 0;
	}

	*paliases = aliases;
	return 0;
}

static int __optimize_alias_bb(scf_list_t** pend, scf_list_t* start, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*        l;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* pointer;
	scf_3ac_operand_t* dst;
	scf_dag_node_t*    dn_pointer;
	scf_dag_node_t*    dn_dereference;
	scf_vector_t*      aliases;

	int ret = 0;

	for (l = start; l != *pend; l = scf_list_next(l)) {

		c  = scf_list_data(l, scf_3ac_code_t, list);

		if (!scf_type_is_assign_dereference(c->op->type)
				&& SCF_OP_DEREFERENCE != c->op->type)
			continue;

		assert(c->srcs && c->srcs->size >= 1);
		pointer        = c->srcs->data[0];
		dn_pointer     = pointer->dag_node;
		aliases        = NULL;
		dn_dereference = NULL;

		if (dn_pointer->var->arg_flag) {
			scf_variable_t* v = dn_pointer->var;
			scf_logd("arg: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
			continue;
		}

		if (dn_pointer->var->global_flag) {
			scf_variable_t* v = dn_pointer->var;
			scf_logd("global: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
			continue;
		}

		if (SCF_OP_DEREFERENCE == c->op->type) {

			assert(c->dsts && 1 == c->dsts->size);
			dst = c->dsts->data[0];
			dn_dereference = dst->dag_node;

			ret = _alias_dereference(&aliases, dn_pointer, c, bb, bb_list_head);
		} else {
			ret = _alias_assign_dereference(&aliases, dn_pointer, c, bb, bb_list_head);
		}

		if (ret < 0)
			return ret;

		if (aliases) {
			scf_logd("bb: %p, bb->index: %d, aliases->size: %d\n", bb, bb->index, aliases->size);
			scf_variable_t* v = dn_pointer->var;
			scf_logd("v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

			ret = _bb_copy_aliases(bb, dn_pointer, dn_dereference, aliases);
			scf_vector_free(aliases);
			aliases = NULL;
			if (ret < 0)
				return ret;

			*pend = l;
			break;
		}
	}

	return 0;
}

static int _optimize_alias_bb(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t* start;
	scf_list_t* end;

	int ret;

	do {
		start = scf_list_head(&bb->code_list_head);
		end   = scf_list_sentinel(&bb->code_list_head);

		ret   = __optimize_alias_bb(&end, start, bb, bb_list_head);

		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (end == scf_list_sentinel(&bb->code_list_head))
			break;

		bb->dereference_flag = 1;

		if (scf_list_next(end) == scf_list_sentinel(&bb->code_list_head))
			break;

		scf_basic_block_t* bb_child = NULL;

		int ret = scf_basic_block_split(bb, &bb_child);
		if (ret < 0)
			return ret;

		bb_child->dereference_flag = 0;
		bb_child->array_index_flag = bb->array_index_flag;

		scf_basic_block_mov_code(scf_list_next(end), bb_child, bb);

		scf_list_add_front(&bb->list, &bb_child->list);

		bb = bb_child;
	} while (1);

	return 0;
}

static int _optimize_pointer_alias(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int ret = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		if (bb->jmp_flag || bb->end_flag || bb->cmp_flag)
			continue;

		if (!bb->dereference_flag)
			continue;

		ret = _optimize_alias_bb(bb, bb_list_head);
		if (ret < 0)
			return ret;
	}

//	scf_basic_block_print_list(bb_list_head);
	ret = 0;
error:
	return ret;
}

scf_optimizer_t  scf_optimizer_pointer_alias =
{
	.name     =  "pointer_alias",

	.optimize =  _optimize_pointer_alias,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

