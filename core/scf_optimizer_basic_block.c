#include"scf_optimizer.h"

static scf_dag_node_t* _func_dag_find_dn(scf_vector_t* dag, scf_dag_node_t* dn_bb, scf_list_t* f_dag)
{
	scf_dag_node_t* dn_func = NULL;
	scf_list_t*     l;

	int i;
	for (i = 0; i < dag->size; i++) {
		dn_func   = dag->data[i];

		scf_variable_t* v1 = dn_func->var;

		if (dn_func->type == dn_bb->type) {

			if (dn_func->var == dn_bb->var)
				break;

			if (scf_dag_dn_same(dn_func, dn_bb))
				break;

			if (dn_bb->node) {
				if (scf_dag_node_like(dn_func, dn_bb->node, f_dag))
					break;
			}
		}
		dn_func = NULL;
	}

	return dn_func;
}

static int _bb_dag_update(scf_basic_block_t* bb, scf_vector_t* dag, scf_list_t* f_dag)
{
	scf_dag_node_t* dn;
	scf_dag_node_t* dn_bb;
	scf_dag_node_t* dn_bb2;
	scf_dag_node_t* dn_func;
	scf_list_t*     l;

	int i;

	while (1) {
		int updated = 0;

		for (l = scf_list_tail(&bb->dag_list_head); l != scf_list_sentinel(&bb->dag_list_head); ) {
			dn = scf_list_data(l, scf_dag_node_t, list);
			l  = scf_list_prev(l);

			if (dn->parents)
				continue;

			if (scf_type_is_var(dn->type))
				continue;

			if (scf_type_is_assign_array_index(dn->type))
				continue;
			if (scf_type_is_assign_pointer(dn->type))
				continue;

			if (scf_type_is_assign(dn->type)
					|| SCF_OP_INC       == dn->type || SCF_OP_DEC       == dn->type
					|| SCF_OP_3AC_INC   == dn->type || SCF_OP_3AC_DEC   == dn->type
					|| SCF_OP_3AC_SETZ  == dn->type || SCF_OP_3AC_SETNZ == dn->type
					|| SCF_OP_3AC_SETLT == dn->type || SCF_OP_3AC_SETLE == dn->type
					|| SCF_OP_3AC_SETGT == dn->type || SCF_OP_3AC_SETGE == dn->type) {

				if (!dn->childs) {
					scf_list_del(&dn->list);
					scf_dag_node_free(dn);
					dn = NULL;

					++updated;
					continue;
				}

				assert(1 == dn->childs->size || 2 == dn->childs->size);

				dn_bb   = dn->childs->data[0];

				dn_func = _func_dag_find_dn(dag, dn_bb, f_dag);
				if (!dn_func) {
					scf_loge("\n");
					return -1;
				}

				if (scf_vector_find(bb->dn_saves, dn_func)
						|| scf_vector_find(bb->dn_resaves, dn_func))
					continue;

				assert(dn_bb->parents && dn_bb->parents->size > 0);

				if (dn != dn_bb->parents->data[dn_bb->parents->size - 1])
					continue;

				if (2      == dn->childs->size) {
					dn_bb2 =  dn->childs->data[1];

					assert(0 == scf_vector_del(dn->childs,      dn_bb2));
					assert(0 == scf_vector_del(dn_bb2->parents, dn));

					if (0 == dn_bb2->parents->size) {
						scf_vector_free(dn_bb2->parents);
						dn_bb2->parents = NULL;
					}
				}

				assert(0 == scf_vector_del(dn->childs,     dn_bb));
				assert(0 == scf_vector_del(dn_bb->parents, dn));

				if (0 == dn_bb->parents->size) {
					scf_vector_free(dn_bb->parents);
					dn_bb->parents = NULL;
				}

				assert(0 == dn->childs->size);
				scf_list_del(&dn->list);
				scf_dag_node_free(dn);
				dn = NULL;

				++updated;

			} else if (SCF_OP_ADD == dn->type || SCF_OP_SUB == dn->type
					|| SCF_OP_MUL == dn->type || SCF_OP_DIV == dn->type
					|| SCF_OP_MOD == dn->type) {
				assert(dn->childs);
				assert(2 == dn->childs->size);

				dn_func = _func_dag_find_dn(dag, dn, f_dag);
				if (!dn_func) {
					scf_loge("\n");
					return -1;
				}

				if (scf_vector_find(bb->dn_saves, dn_func)
						|| scf_vector_find(bb->dn_resaves, dn_func))
					continue;

				for (i = 0; i < dn->childs->size; i++) {
					dn_bb     = dn->childs->data[i];

					assert(0 == scf_vector_del(dn_bb->parents, dn));

					if (0 == dn_bb->parents->size) {
						scf_vector_free(dn_bb->parents);
						dn_bb->parents = NULL;
					}
				}

				scf_list_del(&dn->list);
				scf_dag_node_free(dn);
				dn = NULL;

				++updated;
			}
		}
		scf_logd("bb: %p, updated: %d\n\n", bb, updated);

		if (0 == updated)
			break;
	}
	return 0;
}

static int __optimize_basic_block(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_3ac_operand_t* src;
	scf_3ac_operand_t* src2;
	scf_3ac_operand_t* dst;
	scf_3ac_operand_t* dst2;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    c;
	scf_3ac_code_t*    c2;
	scf_vector_t*      roots;
	scf_vector_t*      dag;
	scf_list_t*        l;
	scf_list_t*        l2;
	scf_list_t         h;

	int ret;
	int i;

	scf_list_init(&h);

	dag = scf_vector_alloc();
	if (!dag)
		return -ENOMEM;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
	     l = scf_list_next(l)) {
		c  = scf_list_data(l, scf_3ac_code_t, list);

		if (c->dsts) {
			assert(1 == c->dsts->size);

			dst = c->dsts->data[0];

			assert(0 == scf_vector_add_unique(dag, dst->dag_node));
		}

		if (c->srcs) {
			for (i  = 0; i < c->srcs->size; i++) {
				src =        c->srcs->data[i];

				assert(0 == scf_vector_add_unique(dag, src->dag_node));
			}
		}
	}

	ret = scf_basic_block_dag2(bb, &bb->dag_list_head);
	if (ret < 0)
		return ret;

	ret = _bb_dag_update(bb, dag, &f->dag_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	roots = scf_vector_alloc();
	if (!roots)
		return -ENOMEM;

	ret = scf_dag_find_roots(&bb->dag_list_head, roots);
	if (ret < 0) {
		scf_vector_free(roots);
		return ret;
	}
	scf_logd("bb: %p, roots->size: %d\n", bb, roots->size);

	for (i = 0; i < roots->size; i++) {
		dn = roots->data[i];

		if (!dn)
			continue;

		ret = scf_dag_expr_calculate(&h, dn);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); l = scf_list_next(l)) {

		c  = scf_list_data(l,  scf_3ac_code_t, list);

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head); l2 = scf_list_next(l2)) {

			c2  = scf_list_data(l2, scf_3ac_code_t, list);

			if (scf_3ac_code_same(c, c2)) {

				if (c->dsts) {
					dst  = c ->dsts->data[0];
					dst2 = c2->dsts->data[0];

					dst->debug_w = dst2->debug_w;
				}

				if (c->srcs) {
					for (i   = 0; i < c ->srcs->size; i++) {
						src  =        c ->srcs->data[i];
						src2 =        c2->srcs->data[i];

						src->debug_w = src2->debug_w;
					}
				}
				break;
			}
		}
//		assert(l2 != scf_list_sentinel(&bb->code_list_head));
	}

	scf_list_clear(&bb->code_list_head, scf_3ac_code_t, list, scf_3ac_code_free);

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {
		c  = scf_list_data(l, scf_3ac_code_t, list);

		if (c->dsts) {
			dst = c->dsts->data[0];

			dn = _func_dag_find_dn(dag, dst->dag_node, &f->dag_list_head);
			if (!dn) {
				scf_loge("\n");
				return -1;
			}

			dst->dag_node = dn;
		}

		if (c->srcs) {
			for (i  = 0; i < c->srcs->size; i++) {
				src = c->srcs->data[i];

				dn = _func_dag_find_dn(dag, src->dag_node, &f->dag_list_head);
				if (!dn) {

					scf_variable_t* v = src->dag_node->var;
					if (v->w)
						scf_loge("v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
					else
						scf_loge("v_%#lx\n", 0xffff & (uintptr_t)v);
					scf_3ac_code_print(c, NULL);
					return -1;
				}

				src->dag_node = dn;
			}
		}

		l = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}

#if 1
	ret = scf_basic_block_active_vars(bb);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
#endif

	scf_dag_node_free_list(&bb->dag_list_head);
	scf_vector_free(roots);
	scf_vector_free(dag);
	return 0;
}

static int _optimize_basic_block(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
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

		if (bb->jmp_flag
				|| bb->ret_flag
				|| bb->end_flag
				|| bb->call_flag
				|| bb->dereference_flag
				|| bb->varg_flag) {
			scf_logd("bb: %p, jmp:%d,ret:%d, end: %d, call:%d, varg:%d, dereference_flag: %d\n",
					bb, bb->jmp_flag, bb->ret_flag, bb->end_flag, bb->call_flag, bb->dereference_flag,
					bb->varg_flag);
			continue;
		}

		ret = __optimize_basic_block(bb, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

//	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_basic_block =
{
	.name     =  "basic_block",

	.optimize =  _optimize_basic_block,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

