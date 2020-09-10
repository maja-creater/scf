#include"scf_optimizer.h"

static int _optimize_loads_saves(scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_dag_node_t*    dn;

	int count;
	int ret;
	int i;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		for (i = 0; i < bb->entry_dn_actives->size; i++) {

			dn = bb->entry_dn_actives->data[i];

			if (scf_vector_find(bb->entry_dn_aliases, dn))
				ret = scf_vector_add_unique(bb->dn_reloads, dn);
			else
				ret = scf_vector_add_unique(bb->dn_loads, dn);
			if (ret < 0)
				return ret;
		}

		for (i = 0; i < bb->exit_dn_actives->size; i++) {

			dn = bb->exit_dn_actives->data[i];

			if (!scf_vector_find(bb->dn_updateds, dn)) {
				if (l != scf_list_head(bb_list_head) || !dn->var->arg_flag)
					continue;
			}

			if (scf_vector_find(bb->exit_dn_aliases, dn))
				ret = scf_vector_add_unique(bb->dn_resaves, dn);
			else
				ret = scf_vector_add_unique(bb->dn_saves, dn);

			if (ret < 0)
				return ret;
		}

		for (i = 0; i < bb->exit_dn_aliases->size; i++) {

			dn = bb->exit_dn_aliases->data[i];

			if (!scf_vector_find(bb->dn_updateds, dn)) {
				if (l != scf_list_head(bb_list_head) || !dn->var->arg_flag)
					continue;
			}

			ret = scf_vector_add_unique(bb->dn_resaves, dn);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_loads_saves =
{
	.name     =  "loads_saves",

	.optimize =  _optimize_loads_saves,
};

