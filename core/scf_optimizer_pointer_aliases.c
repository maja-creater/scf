#include"scf_optimizer.h"

typedef int (*bb_find_pt)(scf_basic_block_t* bb, scf_vector_t* queue);

static int _bb_prev_find_saves(scf_basic_block_t* bb, scf_vector_t* queue)
{
	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->prevs->size; j++) {
		scf_basic_block_t* prev_bb = bb->prevs->data[j];

		int k;
		for (k = 0; k < bb->entry_dn_aliases->size; k++) {
			scf_dag_node_t* dn = bb->entry_dn_aliases->data[k];

			if (scf_vector_find(prev_bb->exit_dn_aliases, dn))
				continue;

			ret = scf_vector_add(prev_bb->exit_dn_aliases, dn);
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

static int _bb_next_find_saves(scf_basic_block_t* bb, scf_vector_t* queue)
{
	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		scf_basic_block_t* next_bb = bb->nexts->data[j];

		int k;
		for (k = 0; k < next_bb->exit_dn_aliases->size; k++) {
			scf_dag_node_t* dn = next_bb->exit_dn_aliases->data[k];

			if (scf_vector_find(bb->exit_dn_aliases, dn))
				continue;

			ret = scf_vector_add(bb->exit_dn_aliases, dn);
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

static int _bb_prev_find_loads(scf_basic_block_t* bb, scf_vector_t* queue)
{
	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->prevs->size; j++) {
		scf_basic_block_t* prev_bb = bb->prevs->data[j];

		int k;
		for (k = 0; k < prev_bb->entry_dn_aliases->size; k++) {
			scf_dag_node_t* dn = prev_bb->entry_dn_aliases->data[k];

			if (scf_vector_find(bb->entry_dn_aliases, dn))
				continue;

			ret = scf_vector_add(bb->entry_dn_aliases, dn);
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

static int _bb_next_find_loads(scf_basic_block_t* bb, scf_vector_t* queue)
{
	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->nexts->size; j++) {
		scf_basic_block_t* next_bb = bb->nexts->data[j];

		int k;
		for (k = 0; k < bb->entry_dn_aliases->size; k++) {
			scf_dag_node_t* dn = bb->entry_dn_aliases->data[k];

			if (scf_vector_find(next_bb->entry_dn_aliases, dn))
				continue;

			ret = scf_vector_add(next_bb->entry_dn_aliases, dn);
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

static void _bb_info_print_list(scf_list_t* h)
{
	scf_list_t*        l;
	scf_list_t*        l2;
	scf_basic_block_t* bb;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head);
				l2 = scf_list_next(l2)) {

			scf_3ac_code_t* c = scf_list_data(l2, scf_3ac_code_t, list);

			scf_3ac_code_print(c, NULL);
		}

		int j;
		for (j = 0; j < bb->entry_dn_aliases->size; j++) {

			scf_dag_node_t* dn = bb->entry_dn_aliases->data[j];
			scf_variable_t* v  = dn->var;

			scf_logw("pointer aliases entry: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		}

		for (j = 0; j < bb->exit_dn_aliases->size; j++) {

			scf_dag_node_t* dn = bb->exit_dn_aliases->data[j];
			scf_variable_t* v  = dn->var;

			scf_logw("pointer aliases exit: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		}
		printf("\n");
	}
}

static int _optimize_pointer_aliases(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head)
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

	do {
		l   = scf_list_tail(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);
		assert(bb->end_flag);

		ret = _bb_search_bfs(bb, _bb_prev_find_saves);
		if (ret < 0)
			return ret;
		count = ret;

		l   = scf_list_head(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);

		ret = _bb_search_bfs(bb, _bb_next_find_saves);
		if (ret < 0)
			return ret;
		count += ret;
	} while (count > 0);

	do {
		l   = scf_list_head(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);
		ret = _bb_search_bfs(bb, _bb_next_find_loads);
		if (ret < 0)
			return ret;
		count = ret;

		l   = scf_list_tail(bb_list_head);
		bb  = scf_list_data(l, scf_basic_block_t, list);
		assert(bb->end_flag);

		ret = _bb_search_bfs(bb, _bb_prev_find_loads);
		if (ret < 0)
			return ret;
		count += ret;
	} while (count > 0);

//	_bb_info_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_pointer_aliases =
{
	.name     =  "pointer_aliases",

	.optimize =  _optimize_pointer_aliases,
};

