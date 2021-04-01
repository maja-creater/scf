#include"scf_optimizer.h"

static int __bb_dfs_tree(scf_basic_block_t* root, scf_vector_t* edges, int* total)
{
	scf_basic_block_t* bb;
	scf_bb_edge_t*     edge;

	int i;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	for (i = 0; i < root->nexts->size; ++i) {

		bb = root->nexts->data[i];

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

	root->depth_first_order = --*total;
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

	for (l = scf_list_tail(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_prev(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		bb->visited_flag = 0;

		if (!bb->jmp_flag)
			++total;
	}
	assert(&bb->list == scf_list_head(bb_list_head));

	f->max_dfo = total - 1;

	return __bb_dfs_tree(bb, f->dfs_tree, &total);
}

static int _bb_cmp_dfo(const void* p0, const void* p1)
{
	scf_basic_block_t* bb0 = *(scf_basic_block_t**)p0;
	scf_basic_block_t* bb1 = *(scf_basic_block_t**)p1;

	if (bb0->depth_first_order < bb1->depth_first_order)
		return -1;
	if (bb0->depth_first_order > bb1->depth_first_order)
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

		if (bb0->depth_first_order < bb1->depth_first_order) {

			int ret = scf_vector_del(dst, bb0);
			if (ret < 0)
				return ret;
			continue;
		}

		if (bb0->depth_first_order > bb1->depth_first_order) {
			++k1;
			continue;
		}

		++k0;
		++k1;
	}

	dst->size = k0;

	return 0;
}

static int _bb_find_dominators(scf_list_t* bb_list_head)
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

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

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

		if (0 == bb->depth_first_order) {

			if (!bb->dominators) {
				bb->dominators = scf_vector_alloc();
				if (!bb->dominators) {
					ret = -ENOMEM;
					goto error;
				}
			} else
				scf_vector_clear(bb->dominators, NULL);

			ret = scf_vector_add(bb->dominators, bb);
			if (ret < 0)
				goto error;

			scf_logd("bb: %p_%d, dom size: %d\n", bb, bb->depth_first_order, bb->dominators->size);
			continue;
		}

		if (bb->dominators)
			scf_vector_free(bb->dominators);

		bb->dominators = scf_vector_clone(all);
		if (!bb->dominators) {
			ret = -ENOMEM;
			goto error;
		}

		scf_logd("bb: %p_%d, dom size: %d\n", bb, bb->depth_first_order, bb->dominators->size);
	}
	printf("\n\n");

	do {
		changed = 0;

		for (i = 1; i < all->size; i++) {
			bb = all->data[i];

			scf_vector_t* dominators = NULL;

			for (j = 0; j < bb->prevs->size; j++) {
				scf_basic_block_t* prev = bb->prevs->data[j];
				scf_logd("bb: %p_%d, prev: %p_%d\n", bb, bb->depth_first_order, prev, prev->depth_first_order);

				if (!dominators) {
					dominators = scf_vector_clone(prev->dominators);
					if (!dominators) {
						ret = -ENOMEM;
						goto error;
					}
					continue;
				}

				ret = _bb_intersection(dominators, prev->dominators);
				if (ret < 0) {
					scf_vector_free(dominators);
					goto error;
				}
			}

			scf_basic_block_t* dom  = NULL;
			scf_basic_block_t* dom1 = bb;

			for (j = 0; j < dominators->size; j++) {
				dom = dominators->data[j];

				if (bb->depth_first_order == dom->depth_first_order)
					break;

				if (bb->depth_first_order < dom->depth_first_order)
					break;
			}
			if (bb->depth_first_order < dom->depth_first_order) {

				for (; j < dominators->size; j++) {
					dom                 = dominators->data[j];
					dominators->data[j] = dom1;
					dom1                = dom;
				}
			}
			if (j == dominators->size) {
				ret = scf_vector_add(dominators, dom1);
				if (ret < 0) {
					scf_vector_free(dominators);
					goto error;
				}
			}

			if (dominators->size != bb->dominators->size) {
				scf_vector_free(bb->dominators);
				bb->dominators = dominators;
				dominators     = NULL;
				++changed;
			} else {
				int k0 = 0;
				int k1 = 0;

				while (k0 < dominators->size && k1 < bb->dominators->size) {

					scf_basic_block_t* dom0 =     dominators->data[k0];
					scf_basic_block_t* dom1 = bb->dominators->data[k1];

					if (dom0->depth_first_order < dom1->depth_first_order)
						++k0;
					else if (dom0->depth_first_order > dom1->depth_first_order)
						++k1;
					else {
						++k0;
						++k1;
					}
				}

				if (k0 == k1) {
					scf_vector_free(dominators);
					dominators = NULL;
				} else {
					scf_vector_free(bb->dominators);
					bb->dominators = dominators;
					dominators     = NULL;
					++changed;
				}
			}
		}
	} while (changed > 0);
#if 1
	for (i = 0; i < all->size; i++) {
		bb = all->data[i];

		int j;
		for (j = 0; j < bb->dominators->size; j++) {

			scf_basic_block_t* dom = bb->dominators->data[j];
			scf_logw("bb: %p_%d, dom: %p_%d\n", bb, bb->depth_first_order, dom, dom->depth_first_order);
		}
		printf("\n");
	}
#endif

	ret = 0;
error:
	scf_vector_free(all);
	return ret;
}

static int _optimize_dominators(scf_function_t* f, scf_list_t* bb_list_head)
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

		scf_logw("bb_%p_%d --> bb_%p_%d\n",
				edge->start, edge->start->depth_first_order,
				edge->end,   edge->end->depth_first_order);
	}

	ret = _bb_find_dominators(bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_dominators =
{
	.name     =  "dominators",

	.optimize =  _optimize_dominators,
};

