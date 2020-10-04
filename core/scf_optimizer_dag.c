#include"scf_optimizer.h"

static int _optimize_dag(scf_function_t* f, scf_list_t* bb_list_head)
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

		ret = scf_basic_block_dag(bb, bb_list_head, &f->dag_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_dag =
{
	.name     =  "dag",

	.optimize =  _optimize_dag,
};

