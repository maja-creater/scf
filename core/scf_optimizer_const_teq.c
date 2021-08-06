#include"scf_optimizer.h"

static int _bb_dfs_del(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_3ac_operand_t* dst;
	scf_basic_block_t* bb2;
	scf_basic_block_t* bb_prev;
	scf_basic_block_t* bb_next;
	scf_3ac_code_t*    c;
	scf_list_t*        l;
	scf_list_t*        l2;
	scf_list_t*        sentinel = scf_list_sentinel(&f->basic_block_list_head);

	for (l  = scf_list_next(&bb->list); l != sentinel; ) {
		bb2 = scf_list_data(l, scf_basic_block_t, list);
		l   = scf_list_next(l);

		if (!bb2->jmp_flag)
			break;

		l2 = scf_list_head(&bb2->code_list_head);
		c  = scf_list_data(l2, scf_3ac_code_t, list);

		assert(0 == scf_vector_del(f->jmps, c));

		scf_list_del(&bb2->list);
		scf_basic_block_free(bb2);
		bb2 = NULL;
	}

	int i;
	for (i = 0; i < bb->nexts->size; ) {
		bb2       = bb->nexts->data[i];

		assert(0 == scf_vector_del(bb ->nexts, bb2));
		assert(0 == scf_vector_del(bb2->prevs, bb));

		if (bb2->prevs->size > 0)
			continue;

		assert(&bb2->list != scf_list_head(&f->basic_block_list_head));

		if (_bb_dfs_del(bb2, f) < 0)
			return -1;
	}

	if (scf_list_prev(&bb->list) != sentinel &&
		scf_list_next(&bb->list) != sentinel) {

		bb_prev = scf_list_data(scf_list_prev(&bb->list), scf_basic_block_t, list);
		bb_next = scf_list_data(scf_list_next(&bb->list), scf_basic_block_t, list);

		if (bb_prev->jmp_flag) {

			l2 = scf_list_head(&bb_prev->code_list_head);
			c  = scf_list_data(l2, scf_3ac_code_t, list);

			assert(c->dsts && 1 == c->dsts->size);
			dst =  c->dsts->data[0];

			if (dst->bb == bb_next) {
				assert(0 == scf_vector_del(f->jmps, c));

				scf_list_del(&bb_prev->list);
				scf_basic_block_free(bb_prev);
				bb_prev = NULL;
			}
		}
	}

	scf_list_del(&bb->list);
	scf_basic_block_free(bb);
	bb = NULL;
	return 0;
}

static int __optimize_const_teq(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_basic_block_t* bb2;

	scf_3ac_operand_t* src;
	scf_3ac_operand_t* dst;
	scf_dag_node_t*    dn;
	scf_variable_t*    v;
	scf_3ac_code_t*    c;
	scf_list_t*        l;

	int flag = -1;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {
		c  = scf_list_data(l, scf_3ac_code_t, list);
	    l  = scf_list_next(l);

		if (SCF_OP_3AC_TEQ == c->op->type) {

			assert(c->srcs && 1 == c->srcs->size);

			src = c->srcs->data[0];
			dn  = src->dag_node;
			v   = dn->var;
	
			if (v->const_flag && 0 == v->nb_pointers && 0 == v->nb_dimentions) {
				flag = !!v->data.i;

				scf_list_del(&c->list);
				scf_3ac_code_free(c);
				c = NULL;

			} else if (v->const_literal_flag) {
				flag = 1;

				scf_list_del(&c->list);
				scf_3ac_code_free(c);
				c = NULL;
			}

		} else if (flag >= 0) {

			int flag2;

			if (SCF_OP_3AC_SETZ == c->op->type)
				flag2 = !flag;
			else if (SCF_OP_3AC_SETNZ == c->op->type)
				flag2 = flag;
			else {
				scf_loge("\n");
				return -EINVAL;
			}

			assert(c->dsts && 1 == c->dsts->size);

			dst = c->dsts->data[0];
			dn  = dst->dag_node;
			v   = dn->var;

			v->const_flag = 1;
			v->data.i     = flag2;

			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;
		}
	}

	if (flag < 0)
		return 0;

	int jmp_flag = 0;
	bb2 = NULL;

	for (l  = scf_list_next(&bb->list); l != scf_list_sentinel(&f->basic_block_list_head); ) {
		bb2 = scf_list_data(l, scf_basic_block_t, list);
		l   = scf_list_next(l);

		if (!bb2->jmp_flag)
			break;

		scf_list_t* l2;

		l2 = scf_list_head(&bb2->code_list_head);
		c  = scf_list_data(l2, scf_3ac_code_t, list);

		if (SCF_OP_3AC_JZ == c->op->type) {

			if (0 == flag) {
				c->op    = scf_3ac_find_operator(SCF_OP_GOTO);
				jmp_flag = 1;
				continue;
			}

		} else if (SCF_OP_3AC_JNZ == c->op->type) {

			if (1 == flag) {
				c->op    = scf_3ac_find_operator(SCF_OP_GOTO);
				jmp_flag = 1;
				continue;
			}

		} else if (SCF_OP_GOTO == c->op->type) {
			jmp_flag = 1;
			continue;
		} else {
			scf_loge("\n");
			return -EINVAL;
		}

		assert(c->dsts && 1 == c->dsts->size);
		dst =  c->dsts->data[0];

		assert(0 == scf_vector_del(dst->bb->prevs, bb));
		assert(0 == scf_vector_del(f->jmps, c));

		scf_list_del(&bb2->list);
		scf_basic_block_free(bb2);
		bb2 = NULL;
	}

	if (jmp_flag && bb2 && !bb2->jmp_flag) {

		assert(0 == scf_vector_del(bb2->prevs, bb));
	}

	int ret;
	int i;

	for (i = 0; i < bb->nexts->size; ) {
		bb2       = bb->nexts->data[i];

		if (scf_vector_find(bb2->prevs, bb)) {
			i++;
			continue;
		}

		assert(0 == scf_vector_del(bb->nexts, bb2));

		if (0 == bb2->prevs->size) {
			if (_bb_dfs_del(bb2, f) < 0)
				return -1;
		}
	}

	return 0;
}

static int _optimize_const_teq(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int count;
	int ret;
	int i;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb  = scf_list_data(l, scf_basic_block_t, list);

		if (!bb->cmp_flag)
			continue;

		ret = __optimize_const_teq(bb, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

//	scf_const_teq_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_const_teq =
{
	.name     =  "const_teq",

	.optimize =  _optimize_const_teq,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

