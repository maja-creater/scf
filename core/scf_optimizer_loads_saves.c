#include"scf_optimizer.h"

static int _optimize_loads_saves(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
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

		ret = scf_basic_block_loads_saves(bb, bb_list_head);
		if (ret < 0)
			return ret;
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_loads_saves =
{
	.name     =  "loads_saves",

	.optimize =  _optimize_loads_saves,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

