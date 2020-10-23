#include"scf_optimizer.h"

static int _optimize_generate_loads_saves(scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    load;
	scf_3ac_code_t*    save;

	int i;
	int ret;

#define SCF_OPTIMIZER_LOAD(load_type) \
			do { \
				load = scf_3ac_alloc_by_dst(load_type, dn); \
				if (!load) { \
					scf_loge("\n"); \
					return -ENOMEM; \
				} \
				load->basic_block = bb; \
				scf_list_add_front(&bb->code_list_head, &load->list); \
			} while (0)

#define SCF_OPTIMIZER_SAVE(save_type, h) \
			do { \
				save = scf_3ac_alloc_by_src(save_type, dn); \
				if (!save) { \
					scf_loge("\n"); \
					return -ENOMEM; \
				} \
				save->basic_block = bb; \
				scf_list_add_tail(h, &save->list); \
			} while (0)

	f->nb_basic_blocks = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

		bb        = scf_list_data(l, scf_basic_block_t, list);

		bb->index = f->nb_basic_blocks++;

		if (bb->generate_flag) {
			for (i = 0; i < bb->dn_loads->size; i++) {
				dn = bb->dn_loads->data[i];
				SCF_OPTIMIZER_LOAD(SCF_OP_3AC_LOAD);
			}

			for (i = 0; i < bb->dn_saves->size; i++) {
				dn = bb->dn_saves->data[i];

				if (bb->group_flag)
					SCF_OPTIMIZER_SAVE(SCF_OP_3AC_SAVE, &bb->save_list_head);
				else
					SCF_OPTIMIZER_SAVE(SCF_OP_3AC_SAVE, &bb->code_list_head);
			}
		}

		for (i = 0; i < bb->dn_reloads->size; i++) {
			dn = bb->dn_reloads->data[i];
			SCF_OPTIMIZER_LOAD(SCF_OP_3AC_RELOAD);
		}

		for (i = 0; i < bb->dn_resaves->size; i++) {
			dn = bb->dn_resaves->data[i];
			SCF_OPTIMIZER_SAVE(SCF_OP_3AC_RESAVE, &bb->code_list_head);
		}
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_generate_loads_saves =
{
	.name     =  "generate_loads_saves",

	.optimize =  _optimize_generate_loads_saves,
};

