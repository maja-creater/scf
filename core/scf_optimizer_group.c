#include"scf_optimizer.h"

int bbg_find_entry_exit(scf_bb_group_t* bbg)
{
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;
	scf_bb_group_t*    bbg2;

	int j;
	int k;

	if (!bbg->entries) {
		bbg ->entries = scf_vector_alloc();
		if (!bbg->entries)
			return -ENOMEM;
	} else
		scf_vector_clear(bbg->entries, NULL);

	if (!bbg->exits) {
		bbg ->exits = scf_vector_alloc();
		if (!bbg->exits)
			return -ENOMEM;
	} else
		scf_vector_clear(bbg->exits, NULL);


	for (j = 0; j < bbg->body->size; j++) {
		bb =        bbg->body->data[j];

		for (k  = 0; k < bb->prevs->size; k++) {
			bb2 =        bb->prevs->data[k];

			if (scf_vector_find(bbg->body, bb2))
				continue;

			if (scf_vector_add_unique(bbg->entries, bb2) < 0)
				return -ENOMEM;
		}

		for (k  = 0; k < bb->nexts->size; k++) {
			bb2 =        bb->nexts->data[k];

			if (scf_vector_find(bbg->body, bb2))
				continue;

			if (scf_vector_add_unique(bbg->exits, bb2) < 0)
				return -ENOMEM;
		}
	}

	return 0;
}

int _optimize_peep_hole(scf_bb_group_t* bbg, scf_basic_block_t* bb)
{
	scf_basic_block_t* bb_prev;
	scf_basic_block_t* bb_next;
	scf_dag_node_t*    dn;

	int i;
	int j;

	for (i = 0; i < bb->prevs->size; i++) {
		bb_prev   = bb->prevs->data[i];

		if (bb_prev->nexts->size != 1)
			return 0;

		if (!scf_vector_find(bbg->body, bb_prev))
			return 0;

		assert(bb_prev->nexts->data[0] == bb);
	}

	for (i = 0; i < bb->dn_reloads->size; ) {
		dn =        bb->dn_reloads->data[i];

		for (j = 0; j < bb->prevs->size; j++) {
			bb_prev   = bb->prevs->data[j];

			if (!scf_vector_find(bb_prev->dn_resaves, dn))
				return 0;
		}

		for (j = 0; j < bb->prevs->size; j++) {
			bb_prev   = bb->prevs->data[j];

			assert(0 == scf_vector_del(bb_prev->dn_resaves, dn));
		}

		assert(0 == scf_vector_del(bb->dn_reloads, dn));

		int ret = scf_vector_add_unique(bb->dn_resaves, dn);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int _optimize_bbg_loads_saves(scf_function_t* f)
{
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;
	scf_basic_block_t* entry;
	scf_basic_block_t* exit;
	scf_bb_group_t*    bbg;
	scf_dag_node_t*    dn;

	int i;
	int j;
	int k;

	for (i  = 0; i < f->bb_groups->size; i++) {
		bbg =        f->bb_groups->data[i];

		bbg->pre = bbg->body->data[0];

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];
#if 1
			int ret = _optimize_peep_hole(bbg, bb);
			if (ret < 0)
				return ret;
#endif
			bb->group_flag = 1;
		}
	}

	return 0;
}

static int _optimize_group(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	return _optimize_bbg_loads_saves(f);
}

scf_optimizer_t  scf_optimizer_group =
{
	.name     =  "group",

	.optimize =  _optimize_group,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

