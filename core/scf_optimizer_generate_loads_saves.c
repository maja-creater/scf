#include"scf_optimizer.h"

static int _optimize_generate_loads_saves(scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int i;
	int ret;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		for (i = 0; i < bb->dn_loads->size; i++) {

			scf_dag_node_t* dn   = bb->dn_loads->data[i];
			scf_variable_t* v    = dn->var;

			scf_3ac_code_t* load = scf_3ac_alloc_by_dst(SCF_OP_3AC_LOAD, dn);
			if (!load) {
				scf_loge("\n");
				return -ENOMEM;
			}
			load->basic_block = bb;

			scf_logd("load: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

			scf_list_add_front(&bb->code_list_head, &load->list);
		}

		for (i = 0; i < bb->dn_saves->size; i++) {

			scf_dag_node_t* dn   = bb->dn_saves->data[i];
			scf_variable_t* v    = dn->var;

			scf_3ac_code_t* save = scf_3ac_alloc_by_src(SCF_OP_3AC_SAVE, dn);
			if (!save) {
				scf_loge("\n");
				return -ENOMEM;
			}
			save->basic_block = bb;

			scf_logd("save: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

			scf_list_add_tail(&bb->code_list_head, &save->list);
		}
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_generate_loads_saves =
{
	.name     =  "generate_loads_saves",

	.optimize =  _optimize_generate_loads_saves,
};

