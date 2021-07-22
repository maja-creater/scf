#include"scf_optimizer.h"

typedef int (*bb_find_pt)(scf_basic_block_t* bb, scf_vector_t* queue);

static int _bb_prev_find(scf_basic_block_t* bb, scf_vector_t* queue)
{
	scf_basic_block_t* prev_bb;
	scf_dag_node_t*    dn;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->prevs->size; j++) {
		prev_bb   = bb->prevs->data[j];

		int k;
		for (k = 0; k < bb->entry_dn_actives->size; k++) {
			dn =        bb->entry_dn_actives->data[k];

			if (scf_vector_find(prev_bb->exit_dn_actives, dn))
				continue;

			ret = scf_vector_add(prev_bb->exit_dn_actives, dn);
			if (ret < 0)
				return ret;
			++count;
		}

		ret = scf_vector_add(queue, prev_bb);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _bb_next_find(scf_basic_block_t* bb, scf_vector_t* queue)
{
	scf_basic_block_t* next_bb;
	scf_dag_node_t*    dn;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		next_bb   = bb->nexts->data[j];

		int k;
		for (k = 0; k < next_bb->exit_dn_actives->size; k++) {
			dn =        next_bb->exit_dn_actives->data[k];

			if (scf_vector_find(next_bb->dn_updateds, dn))
				continue;

			if (scf_vector_find(bb->exit_dn_actives, dn))
				continue;

			ret = scf_vector_add(bb->exit_dn_actives, dn);
			if (ret < 0)
				return ret;
			++count;
		}

		ret = scf_vector_add(queue, next_bb);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _bb_search_bfs(scf_basic_block_t* root, bb_find_pt find)
{
	if (!root)
		return -EINVAL;

	scf_vector_t* queue   = scf_vector_alloc();
	if (!queue)
		return -ENOMEM;

	scf_vector_t* checked = scf_vector_alloc();
	if (!queue) {
		scf_vector_free(queue);
		return -ENOMEM;
	}

	int ret = scf_vector_add(queue, root);
	if (ret < 0)
		goto failed;

	int count = 0;
	int i     = 0;

	while (i < queue->size) {
		scf_basic_block_t* bb = queue->data[i];

		int j;
		for (j = 0; j < checked->size; j++) {
			if (bb == checked->data[j])
				goto next;
		}

		ret = scf_vector_add(checked, bb);
		if (ret < 0)
			goto failed;

		ret = find(bb, queue);
		if (ret < 0)
			goto failed;
		count += ret;
next:
		i++;
	}

	ret = count;
failed:
	scf_vector_free(queue);
	scf_vector_free(checked);
	queue   = NULL;
	checked = NULL;
	return ret;
}

static int _optimize_active_vars(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
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

		ret = scf_basic_block_active_vars(bb);
		if (ret < 0)
			return ret;
	}

	do {
		l   = scf_list_tail(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);
		assert(bb->end_flag);

		ret = _bb_search_bfs(bb, _bb_prev_find);
		if (ret < 0)
			return ret;
		count = ret;

		l   = scf_list_head(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);

		ret = _bb_search_bfs(bb, _bb_next_find);
		if (ret < 0)
			return ret;
		count += ret;
	} while (count > 0);

//	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_active_vars =
{
	.name     =  "active_vars",

	.optimize =  _optimize_active_vars,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

