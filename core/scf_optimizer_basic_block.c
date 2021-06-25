#include"scf_optimizer.h"

static scf_dag_node_t* _func_dag_find_dn(scf_list_t* func_dag, scf_dag_node_t* dn_bb)
{
	scf_dag_node_t* dn_func = NULL;
	scf_list_t*     l;

	for (l = scf_list_head(func_dag); l != scf_list_sentinel(func_dag); l = scf_list_next(l)) {

		dn_func = scf_list_data(l, scf_dag_node_t, list);

		if (dn_func->type == dn_bb->type) {

			if (dn_func->var == dn_bb->var)
				break;

			if (scf_dag_dn_same(dn_func, dn_bb))
				break;

			if (dn_bb->node && scf_dag_node_same(dn_func, dn_bb->node))
				break;
		}
		dn_func = NULL;
	}

	if (!dn_func) {
		scf_variable_t* v = dn_bb->var;
		if (v->w)
			scf_loge("v_%d_%d/%s, v: %p, dn: %p, node: %p, dn_bb->type: %d\n", v->w->line, v->w->pos, v->w->text->data, v, dn_bb, dn_bb->node, dn_bb->type);
		else
			scf_loge("v_%#lx\n", 0xffff & (uintptr_t)v);
	}

	return dn_func;
}

static int _bb_dag_update(scf_basic_block_t* bb, scf_function_t* f)
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
#if 0
			if (dn->var) {
				if (dn->var->w)
					scf_logw("roots: dn: %p, type: %d, 3AC_INC: %d, v_%d_%d/%s\n",
							dn, dn->type, SCF_OP_3AC_INC, dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
				else
					scf_logw("roots: v_%#lx\n", 0xffff & (uintptr_t)dn->var);
			} else
				scf_logw("roots: dn_%p\n", dn);
#endif
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

				dn_func = _func_dag_find_dn(&f->dag_list_head, dn_bb);
				if (!dn_func) {
					scf_loge("\n");
					return -1;
				}
#if 0
				if (dn_bb->var) {
					scf_variable_t* v = dn_bb->var;

					if (dn_bb->var->w)
						scf_loge("roots: dn_bb: %p, v_%d_%d/%s\n", dn_bb, v->w->line, v->w->pos, v->w->text->data);
					else
						scf_loge("roots: v_%#lx\n", 0xffff & (uintptr_t)dn_bb->var);
				} else
					scf_loge("roots: dn_bb_%p\n", dn_bb);
#endif
				if (scf_vector_find(bb->dn_saves, dn_func)
						|| scf_vector_find(bb->dn_resaves, dn_func))
					continue;

				assert(dn_bb->parents && dn_bb->parents->size > 0);

				if (dn != dn_bb->parents->data[dn_bb->parents->size - 1])
					continue;
#if 0
				int j;
				for (j = 0; j < dn_bb->parents->size; j++) {
					scf_dag_node_t* p = dn_bb->parents->data[j];
					scf_loge("j: %d, p: %p, dn: %p\n", j, p, dn);
				}
				printf("\n");
#endif
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
	scf_dag_node_t* dn;
	scf_dag_node_t* dn_bb;
	scf_dag_node_t* dn_func;
	scf_vector_t*   roots;
	scf_list_t*     l;
	scf_list_t      h;

	int ret;
	int i;

	scf_list_init(&h);

	ret = scf_basic_block_dag2(bb, &bb->dag_list_head);
	if (ret < 0)
		return ret;

	ret = _bb_dag_update(bb, f);
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
#if 0
	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_print(c, NULL);

		l = scf_list_next(l);
	}

	scf_loge("bb: %p\n\n", bb);
	return 0;
#endif
	scf_list_clear(&bb->code_list_head, scf_3ac_code_t, list, scf_3ac_code_free);

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

//		scf_3ac_code_print(c, NULL);

		if (c->dsts) {
			scf_3ac_operand_t* dst = c->dsts->data[0];

			dn_func = _func_dag_find_dn(&f->dag_list_head, dst->dag_node);
			if (!dn_func) {
				scf_loge("\n");
				return -1;
			}

			dst->dag_node = dn_func;
		}

		if (c->srcs) {
			scf_3ac_operand_t* src;

			for (i  = 0; i < c->srcs->size; i++) {
				src = c->srcs->data[i];

				scf_variable_t* v = src->dag_node->var;

				dn_func = _func_dag_find_dn(&f->dag_list_head, src->dag_node);
				if (!dn_func) {
					if (v->w)
						scf_loge("v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
					else
						scf_loge("v_%#lx\n", 0xffff & (uintptr_t)v);
					scf_3ac_code_print(c, NULL);
					return -1;
				}

				src->dag_node = dn_func;
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

		if (bb->jmp_flag || bb->ret_flag || bb->end_flag || bb->call_flag
				|| bb->dereference_flag) {
			if (bb->ret_flag)
				scf_logw("jmp bb: %p\n\n", bb);
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
};

