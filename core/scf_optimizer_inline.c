#include"scf_optimizer.h"

static int _arg_cmp(const void* p0, const void* p1)
{
	scf_node_t*     n0 = (scf_node_t*)p0;
	scf_node_t*     n1 = (scf_node_t*)p1;

	scf_variable_t* v0 = _scf_operand_get(n0);
	scf_variable_t* v1 = _scf_operand_get(n1);

	return v0 != v1;
}

static int _find_argv(scf_vector_t* argv, scf_vector_t* operands)
{
	scf_3ac_operand_t* operand;
	scf_variable_t*    v;

	int i;
	for (i = 0; i < operands->size; i++) {
		operand   = operands->data[i];
		v         = _scf_operand_get(operand->node);

		if (!v->arg_flag)
			continue;

		if (scf_vector_find_cmp(argv, operand->node, _arg_cmp))
			continue;

		if (scf_vector_add(argv, operand->node) < 0)
			return -ENOMEM;
	}

	return 0;
}

static int _copy_codes(scf_list_t* hbb, scf_vector_t* argv, scf_3ac_code_t* c, scf_function_t* f, scf_function_t* f2)
{
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;
	scf_basic_block_t* bb3;
	scf_3ac_operand_t* src;
	scf_3ac_operand_t* dst;
	scf_3ac_code_t*    c2;
	scf_3ac_code_t*    c3;
	scf_variable_t*    v;
	scf_list_t*        l;
	scf_list_t*        l2;
	scf_list_t*        l3;

	for (l = scf_list_head(&f2->basic_block_list_head); l != scf_list_sentinel(&f2->basic_block_list_head); ) {
		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb2 = scf_basic_block_alloc();
		if (!bb2)
			return -ENOMEM;
		bb2->index = bb->index;

		scf_vector_free(bb2->prevs);
		scf_vector_free(bb2->nexts);

		bb2->prevs = scf_vector_clone(bb->prevs);
		bb2->nexts = scf_vector_clone(bb->nexts);

		bb2->call_flag        = bb->call_flag;
		bb2->cmp_flag         = bb->cmp_flag;
		bb2->jmp_flag         = bb->jmp_flag;
		bb2->jcc_flag         = bb->jcc_flag;
		bb2->ret_flag         = 0;
		bb2->end_flag         = 0;

		bb2->varg_flag        = bb->varg_flag;
		bb2->jmp_dst_flag     = bb->jmp_dst_flag;

		bb2->dereference_flag = bb->dereference_flag;
		bb2->array_index_flag = bb->array_index_flag;

		scf_list_add_tail(hbb, &bb2->list);

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head); ) {
			c2  = scf_list_data(l2, scf_3ac_code_t, list);
			l2  = scf_list_next(l2);

			if (SCF_OP_3AC_END == c2->op->type)
				continue;

			if (SCF_OP_RETURN == c2->op->type && c2->srcs && c->dsts) {
				int i;
				int min = c2->srcs->size;

				if (c2->srcs->size > c->dsts->size)
					min = c->dsts->size;

				for (i  = 0; i < min; i++) {
					src = c2->srcs->data[i];
					dst = c ->dsts->data[i];
					v   = _scf_operand_get(dst->node);

					dst->node->type       = v->type;
					dst->node->var        = v;
					dst->node->result     = NULL;
					dst->node->split_flag = 0;

					c3  = scf_3ac_code_NN(SCF_OP_ASSIGN, &dst->node, 1, &src->node, 1);
					scf_list_add_tail(&bb2->code_list_head, &c3->list);
				}
			} else {
				c3  = scf_3ac_code_clone(c2);
				if (!c3)
					return -ENOMEM;
				scf_list_add_tail(&bb2->code_list_head, &c3->list);
			}
			c3->basic_block = bb2;

			if (scf_type_is_jmp(c3->op->type))
				continue;

			if (c3->srcs) {
				int ret = _find_argv(argv, c3->srcs);
				if (ret < 0)
					return ret;
			}

			if (c3->dsts) {
				int ret = _find_argv(argv, c3->dsts);
				if (ret < 0)
					return ret;
			}
		}
	}

	for (l = scf_list_head(hbb); l != scf_list_sentinel(hbb); l = scf_list_next(l)) {
		bb = scf_list_data(l, scf_basic_block_t, list);

		int i;
		for (i  = 0; i < bb->prevs->size; i++) {
			bb2 =        bb->prevs->data[i];

			for (l3 = scf_list_head(hbb); l3 != scf_list_sentinel(hbb); l3 = scf_list_next(l3)) {
				bb3 = scf_list_data(l3, scf_basic_block_t, list);

				if (bb2->index == bb3->index) {
					bb->prevs->data[i] = bb3;
					break;
				}
			}
		}

		for (i  = 0; i < bb->nexts->size; i++) {
			bb2 =        bb->nexts->data[i];

			for (l3 = scf_list_head(hbb); l3 != scf_list_sentinel(hbb); l3 = scf_list_next(l3)) {
				bb3 = scf_list_data(l3, scf_basic_block_t, list);

				if (bb2->index == bb3->index) {
					bb->nexts->data[i] = bb3;
					break;
				}
			}
		}

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head); ) {
			c2  = scf_list_data(l2, scf_3ac_code_t, list);
			l2  = scf_list_next(l2);

			if (!scf_type_is_jmp(c2->op->type))
				continue;

			dst = c2->dsts->data[0];

			for (l3 = scf_list_head(hbb); l3 != scf_list_sentinel(hbb); l3 = scf_list_next(l3)) {
				bb3 = scf_list_data(l3, scf_basic_block_t, list);

				if (dst->bb->index == bb3->index) {
					dst->bb = bb3;
					break;
				}
			}

			if (scf_vector_add(f->jmps, c2) < 0)
				return -ENOMEM;
		}
	}

	return 0;
}

static int _find_local_vars(scf_node_t* node, void* arg, scf_vector_t* results)
{
	scf_block_t* b = (scf_block_t*)node;

	if ((SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type) && b->scope) {

		int i;
		for (i = 0; i < b->scope->vars->size; i++) {

			scf_variable_t* var = b->scope->vars->data[i];

			int ret = scf_vector_add(results, var);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static int _do_inline(scf_ast_t* ast, scf_3ac_code_t* c, scf_basic_block_t** pbb, scf_function_t* f, scf_function_t* f2)
{
	scf_basic_block_t* bb = *pbb;
	scf_basic_block_t* bb2;
	scf_basic_block_t* bb_next;
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c2;
	scf_variable_t*    v2;
	scf_vector_t*      argv;
	scf_node_t*        node;
	scf_list_t*        l;

	scf_list_t         hbb;

	assert(c->srcs->size - 1 == f2->argv->size);

	scf_list_init(&hbb);

	argv = scf_vector_alloc();
	if (!argv)
		return -ENOMEM;

	_copy_codes(&hbb, argv, c, f, f2);

	int i;
	int j;
	for (i = 0; i < argv->size; i++) {
		node      = argv->data[i];

		for (j  = 0; j < f2->argv->size; j++) {
			v2  =        f2->argv->data[j];

			if (_scf_operand_get(node) == v2)
				break;
		}

		assert(j < f2->argv->size);

		src = c->srcs->data[j + 1];

		c2  = scf_3ac_code_NN(SCF_OP_ASSIGN, &node, 1, &src->node, 1);

		scf_list_add_tail(&c->list, &c2->list);

		c2->basic_block = bb;
	}

	scf_vector_clear(argv, NULL);

	int ret = scf_node_search_bfs((scf_node_t*)f2, NULL, argv, -1, _find_local_vars);
	if (ret < 0) {
		scf_vector_free(argv);
		return -ENOMEM;
	}

	for (i = 0; i < argv->size; i++) {
		v2 =        argv->data[i];

		ret = scf_vector_add_unique(f->scope->vars, v2);
		if (ret < 0) {
			scf_vector_free(argv);
			return -ENOMEM;
		}
	}

	scf_vector_free(argv);
	argv = NULL;

	l    = scf_list_tail(&hbb);
	bb2  = scf_list_data(l, scf_basic_block_t, list);
	*pbb = bb2;

	SCF_XCHG(bb->nexts, bb2->nexts);
	bb2->end_flag = 0;

	for (i = 0; i < bb2->nexts->size; i++) {
		bb_next   = bb2->nexts->data[i];

		int j;
		for (j = 0; j < bb_next->prevs->size; j++) {

			if (bb_next->prevs->data[j] == bb) {
				bb_next->prevs->data[j] =  bb2;
				break;
			}
		}
	}

	int nblocks = 0;

	while (l != scf_list_sentinel(&hbb)) {

		bb2   = scf_list_data(l, scf_basic_block_t, list);
		l     = scf_list_prev(l);

		scf_list_del(&bb2->list);
		scf_list_add_front(&bb->list, &bb2->list);

		nblocks++;
	}

	if (bb2->jmp_flag || bb2->jmp_dst_flag) {

		if (scf_vector_add(bb->nexts, bb2) < 0)
			return -ENOMEM;

		if (scf_vector_add(bb2->prevs, bb) < 0)
			return -ENOMEM;

	} else {
		for (l = scf_list_head(&bb2->code_list_head); l != scf_list_sentinel(&bb2->code_list_head); ) {

			c2 = scf_list_data(l, scf_3ac_code_t, list);
			l  = scf_list_next(l);

			scf_list_del(&c2->list);
			scf_list_add_tail(&c->list, &c2->list);

			c2->basic_block = bb;

			if (SCF_OP_CALL == c2->op->type)
				bb->call_flag = 1;
		}

		SCF_XCHG(bb->nexts, bb2->nexts);

		for (i = 0; i < bb->nexts->size; i++) {
			bb_next   = bb->nexts->data[i];

			int j;
			for (j = 0; j < bb_next->prevs->size; j++) {

				if (bb_next->prevs->data[j] == bb2) {
					bb_next->prevs->data[j] =  bb;
					break;
				}
			}
		}

		scf_list_del(&bb2->list);
		scf_basic_block_free(bb2);
		bb2 = NULL;

		if (1 == nblocks)
			*pbb = bb;
	}
	return 0;
}

static int _optimize_inline2(scf_ast_t* ast, scf_function_t* f)
{
	scf_basic_block_t* bb;
	scf_basic_block_t* bb_cur;
	scf_basic_block_t* bb2;
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_variable_t*    v;
	scf_function_t*    f2;
	scf_list_t*        l;
	scf_list_t*        l2;

	bb_cur = NULL;

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head); ) {
		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		if (!bb->call_flag)
			continue;

		int call_flag = 0;

		bb_cur = bb;

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head); ) {
			c   = scf_list_data(l2, scf_3ac_code_t, list);
			l2  = scf_list_next(l2);

			if (bb_cur != bb) {
				scf_list_del(&c->list);
				scf_list_add_tail(&bb_cur->code_list_head, &c->list);

				c->basic_block = bb_cur;
			}

			if (SCF_OP_CALL != c->op->type)
				continue;

			call_flag = 1;

			src = c->srcs->data[0];
			v   = _scf_operand_get(src->node);

			if (!v->const_literal_flag)
				continue;

			f2 = v->func_ptr;

			if (!f2->node.define_flag)
				continue;

			if (!f2->inline_flag)
				continue;

			if (f2->vargs_flag)
				continue;

			if (f2->nb_basic_blocks > 10)
				continue;
#if 1
			bb2 = bb_cur;
			bb_cur->call_flag = 0;

			int ret = _do_inline(ast, c, &bb_cur, f, f2);
			if (ret < 0)
				return ret;

			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;

			bb2->call_flag |= call_flag;
			call_flag       = 0;

			if (bb2->ret_flag) {
				bb2->ret_flag    = 0;
				bb_cur->ret_flag = 1;
			}
#endif
		}

		bb_cur->call_flag |= call_flag;
		call_flag = 0;
	}

#if 0
	if (bb_cur)
		scf_basic_block_print_list(&f->basic_block_list_head);
#endif
	return 0;
}

static int _optimize_inline(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!ast || !functions || functions->size <= 0)
		return -EINVAL;

	int i;
	for (i = 0; i < functions->size; i++) {

		int ret = _optimize_inline2(ast, functions->data[i]);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_inline =
{
	.name     =  "inline",

	.optimize =  _optimize_inline,

	.flags    = SCF_OPTIMIZER_GLOBAL,
};

