#include"scf_optimizer.h"

static int __optimize_basic_block(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_vector_t* roots = scf_vector_alloc();
	if (!roots)
		return -ENOMEM;

	int ret = scf_dag_find_roots(&bb->dag_list_head, roots);
	if (ret < 0) {
		scf_vector_free(roots);
		return ret;
	}

	scf_dag_node_t* dn;
	scf_dag_node_t* dn_bb;
	scf_dag_node_t* dn_func;

	scf_logw("bb: %p\n", bb);

	int i;
	for (i = 0; i < roots->size; i++) {
		dn = roots->data[i];
#if 1
		if (dn->var) {
			if (dn->var->w)
				scf_logw("roots: v_%d_%d/%s\n", dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
			else
				scf_logw("roots: v_%#lx\n", 0xffff & (uintptr_t)dn->var);
		} else {
			scf_logw("roots: dn_%p\n", dn);
		}
#endif
		if (scf_type_is_assign(dn->type)
				|| SCF_OP_INC      == dn->type
				|| SCF_OP_DEC      == dn->type
				|| SCF_OP_INC_POST == dn->type
				|| SCF_OP_DEC_POST == dn->type) {

			assert(dn->childs && dn->childs->size > 0);

			dn_bb = dn->childs->data[0];

			dn_func = scf_dag_find_dn(&f->dag_list_head, dn_bb);
			if (!dn_func) {
				scf_loge("\n");
				return -1;
			}

			if (scf_vector_find(bb->dn_saves, dn_func)
					|| scf_vector_find(bb->dn_resaves, dn_func))
				continue;

			roots->data[i] = NULL;
		}
	}
	printf("\n");

	scf_list_t  h;
	scf_list_t* l;
	scf_list_init(&h);

	scf_logw("\n");
	for (i = 0; i < roots->size; i++) {
		dn = roots->data[i];

		if (!dn) {
			scf_logw("\n");
			continue;
		}

		ret = scf_dag_expr_calculate(&h, dn);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_print(c, NULL);
	}
	scf_logw("\n\n");

	scf_vector_free(roots);
	return 0;
}

static int _optimize_basic_block(scf_function_t* f, scf_list_t* bb_list_head)
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

		if (bb->jmp_flag || bb->cmp_flag || bb->call_flag || bb->dereference_flag) {
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

