#include"scf_optimizer.h"

static int __bb_dfs_tree(scf_basic_block_t* root, scf_vector_t* edges, int* total)
{
	scf_basic_block_t* bb;
	scf_bb_edge_t*     edge;

	int i;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	for (i = 0; i < root->prevs->size; ++i) {

		bb = root->prevs->data[i];

		if (bb->visited_flag)
			continue;

		edge = malloc(sizeof(scf_bb_edge_t));
		if (!edge)
			return -ENOMEM;

		edge->start = root;
		edge->end   = bb;

		ret = scf_vector_add(edges, edge);
		if ( ret < 0)
			return ret;

		ret = __bb_dfs_tree(bb, edges, total);
		if ( ret < 0)
			return ret;
	}

	root->dfo_reverse = --*total;
	return 0;
}

static int _bb_dfs_tree(scf_list_t* bb_list_head, scf_function_t* f)
{
	if (!bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int total = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		bb->visited_flag = 0;

		if (!bb->jmp_flag)
			++total;
	}
	assert(&bb->list == scf_list_tail(bb_list_head));

	f->max_dfo_reverse = total - 1;

	return __bb_dfs_tree(bb, f->dfs_tree, &total);
}

static int _bb_cmp_dfo(const void* p0, const void* p1)
{
	scf_basic_block_t* bb0 = *(scf_basic_block_t**)p0;
	scf_basic_block_t* bb1 = *(scf_basic_block_t**)p1;

	if (bb0->dfo_reverse < bb1->dfo_reverse)
		return -1;
	if (bb0->dfo_reverse > bb1->dfo_reverse)
		return 1;
	return 0;
}

static int _bb_intersection(scf_vector_t* dst, scf_vector_t* src)
{
	int k0 = 0;
	int k1 = 0;

	while (k0 < dst->size && k1 < src->size) {

		scf_basic_block_t* bb0 = dst->data[k0];
		scf_basic_block_t* bb1 = src->data[k1];

		if (bb0->dfo_reverse < bb1->dfo_reverse) {

			int ret = scf_vector_del(dst, bb0);
			if (ret < 0)
				return ret;
			continue;
		}

		if (bb0->dfo_reverse > bb1->dfo_reverse) {
			++k1;
			continue;
		}

		++k0;
		++k1;
	}

	dst->size = k0;

	return 0;
}

static int _bb_find_dominators_reverse(scf_list_t* bb_list_head)
{
	if (!bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_vector_t*      all;

	int i;
	int j;
	int ret;
	int changed;

	all = scf_vector_alloc();
	if (!all)
		return -ENOMEM;

	for (l = scf_list_tail(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_prev(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		if (bb->jmp_flag)
			continue;

		ret = scf_vector_add(all, bb);
		if (ret < 0)
			goto error;
	}

	scf_vector_qsort(all, _bb_cmp_dfo);

	for (i = 0; i < all->size; i++) {
		bb = all->data[i];

		scf_vector_qsort(bb->prevs, _bb_cmp_dfo);
		scf_vector_qsort(bb->nexts, _bb_cmp_dfo);

		if (0 == bb->dfo_reverse) {

			if (!bb->dominators_reverse) {
				bb->dominators_reverse = scf_vector_alloc();
				if (!bb->dominators_reverse) {
					ret = -ENOMEM;
					goto error;
				}
			} else
				scf_vector_clear(bb->dominators_reverse, NULL);

			ret = scf_vector_add(bb->dominators_reverse, bb);
			if (ret < 0)
				goto error;

			scf_logd("bb: %p_%d, dom size: %d\n", bb, bb->dfo_reverse, bb->dominators_reverse->size);
			continue;
		}

		if (bb->dominators_reverse)
			scf_vector_free(bb->dominators_reverse);

		bb->dominators_reverse = scf_vector_clone(all);
		if (!bb->dominators_reverse) {
			ret = -ENOMEM;
			goto error;
		}

		scf_logd("bb: %p_%d, dom size: %d\n", bb, bb->dfo_reverse, bb->dominators_reverse->size);
	}

	do {
		changed = 0;

		for (i = 1; i < all->size; i++) {
			bb = all->data[i];

			scf_vector_t* dominators_reverse = NULL;

			for (j = 0; j < bb->nexts->size; j++) {
				scf_basic_block_t* next = bb->nexts->data[j];
				scf_logd("bb: %p_%d, next: %p_%d\n", bb, bb->dfo_normal, next, next->dfo_normal);

				if (!dominators_reverse) {
					dominators_reverse = scf_vector_clone(next->dominators_reverse);
					if (!dominators_reverse) {
						ret = -ENOMEM;
						goto error;
					}
					continue;
				}

				ret = _bb_intersection(dominators_reverse, next->dominators_reverse);
				if (ret < 0) {
					scf_vector_free(dominators_reverse);
					goto error;
				}
			}

			scf_logd("bb: %p, dominators_reverse: %p, bb->nexts->size: %d\n",
					bb, dominators_reverse, bb->nexts->size);

			scf_basic_block_t* dom  = NULL;
			scf_basic_block_t* dom1 = bb;

			for (j = 0; j < dominators_reverse->size; j++) {
				dom = dominators_reverse->data[j];

				if (bb->dfo_reverse == dom->dfo_reverse)
					break;

				if (bb->dfo_reverse < dom->dfo_reverse)
					break;
			}
			if (bb->dfo_reverse < dom->dfo_reverse) {

				for (; j < dominators_reverse->size; j++) {
					dom                 = dominators_reverse->data[j];
					dominators_reverse->data[j] = dom1;
					dom1                = dom;
				}
			}
			if (j == dominators_reverse->size) {
				ret = scf_vector_add(dominators_reverse, dom1);
				if (ret < 0) {
					scf_vector_free(dominators_reverse);
					goto error;
				}
			}

			if (dominators_reverse->size != bb->dominators_reverse->size) {
				scf_vector_free(bb->dominators_reverse);
				bb->dominators_reverse = dominators_reverse;
				dominators_reverse     = NULL;
				++changed;
			} else {
				int k0 = 0;
				int k1 = 0;

				while (k0 < dominators_reverse->size && k1 < bb->dominators_reverse->size) {

					scf_basic_block_t* dom0 =     dominators_reverse->data[k0];
					scf_basic_block_t* dom1 = bb->dominators_reverse->data[k1];

					if (dom0->dfo_reverse < dom1->dfo_reverse)
						++k0;
					else if (dom0->dfo_reverse > dom1->dfo_reverse)
						++k1;
					else {
						++k0;
						++k1;
					}
				}

				if (k0 == k1) {
					scf_vector_free(dominators_reverse);
					dominators_reverse = NULL;
				} else {
					scf_vector_free(bb->dominators_reverse);
					bb->dominators_reverse = dominators_reverse;
					dominators_reverse     = NULL;
					++changed;
				}
			}
		}
	} while (changed > 0);
#if 0
	for (i = 0; i < all->size; i++) {
		bb = all->data[i];

		int j;
		for (j = 0; j < bb->dominators_reverse->size; j++) {

			scf_basic_block_t* dom = bb->dominators_reverse->data[j];
			scf_logd("bb: %p_%d, dom: %p_%d\n", bb, bb->dfo_reverse, dom, dom->dfo_reverse);
		}
		printf("\n");
	}
#endif

	ret = 0;
error:
	scf_vector_free(all);
	return ret;
}

static int _optimize_dominators_reverse(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int ret;
	int i;

	scf_vector_clear(f->dfs_tree, free);

	ret = _bb_dfs_tree(bb_list_head, f);
	if (ret < 0)
		return ret;

	for (i = 0; i < f->dfs_tree->size; i++) {
		scf_bb_edge_t* edge = f->dfs_tree->data[i];

		scf_logd("bb_%p_%d --> bb_%p_%d\n",
				edge->start, edge->start->dfo_reverse,
				edge->end,   edge->end->dfo_reverse);
	}

	ret = _bb_find_dominators_reverse(bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

//	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_dominators_reverse =
{
	.name     =  "dominators_reverse",

	.optimize =  _optimize_dominators_reverse,
};

