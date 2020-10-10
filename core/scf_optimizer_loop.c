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

			scf_logw("bb: %p_%d, dom size: %d\n", bb, bb->depth_first_order, bb->dominators->size);
			continue;
		}

		if (bb->dominators)
			scf_vector_free(bb->dominators);

		bb->dominators = scf_vector_clone(all);
		if (!bb->dominators) {
			ret = -ENOMEM;
			goto error;
		}

		scf_logw("bb: %p_%d, dom size: %d\n", bb, bb->depth_first_order, bb->dominators->size);
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
#if 0
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

static int __bb_dfs_loop2(scf_basic_block_t* root, scf_vector_t* loop)
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

		ret = scf_vector_add(loop, bb);
		if ( ret < 0)
			return ret;

		ret = __bb_dfs_loop2(bb, loop);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

static int __bb_dfs_loop(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_basic_block_t* dom, scf_vector_t* loop)
{
	scf_list_t*        l;
	scf_basic_block_t* bb2;

	int ret = scf_vector_add(loop, bb);
	if (ret < 0)
		return ret;

	ret = scf_vector_add(loop, dom);
	if (ret < 0)
		return ret;

	if (dom == bb)
		return 0;

	for (l = scf_list_tail(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_prev(l)) {

		bb2 = scf_list_data(l, scf_basic_block_t, list);

		if (bb2->jmp_flag)
			continue;

		bb2->visited_flag = 0;
	}
	dom->visited_flag = 1;

	return __bb_dfs_loop2(bb, loop);
}

static int _bb_loop_cmp(const void* p0, const void* p1)
{
	scf_bb_group_t* bbg0 = *(scf_bb_group_t**)p0;
	scf_bb_group_t* bbg1 = *(scf_bb_group_t**)p1;

	if (bbg0->body->size < bbg1->body->size)
		return -1;

	if (bbg0->body->size > bbg1->body->size)
		return 1;
	return 0;
}

static int _bb_dfs_loop(scf_list_t* bb_list_head, scf_vector_t* loops)
{
	if (!bb_list_head || !loops)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		if (bb->jmp_flag)
			continue;

		int ret;

		int i = 0;
		int j = 0;
		while (i < bb->nexts->size && j < bb->dominators->size) {

			scf_basic_block_t* next = bb->nexts->data[i];
			scf_basic_block_t* dom  = bb->dominators->data[j];

			if (next->depth_first_order < dom->depth_first_order) {
				++i;
				continue;
			}

			if (next->depth_first_order > dom->depth_first_order) {
				++j;
				continue;
			}

			scf_vector_t* loop = scf_vector_alloc();
			if (!loop)
				return -ENOMEM;

			ret = __bb_dfs_loop(bb_list_head, bb, dom, loop);
			if (ret < 0) {
				scf_loge("\n");
				scf_vector_free(loop);
				return ret;
			}

			scf_bb_group_t* bbg = calloc(1, sizeof(scf_bb_group_t));
			if (!bbg) {
				scf_vector_free(loop);
				return -ENOMEM;
			}
			bbg->loop_layers = 1;
			bbg->body        = loop;
			loop             = NULL;

			scf_basic_block_t* entry;
			scf_basic_block_t* exit;
			int k;

			for (k = 0; k < dom->prevs->size; k++) {
				entry     = dom->prevs->data[k];

				if (scf_vector_find(bbg->body, entry))
					continue;

				if (!bbg->entry || bbg->entry->depth_first_order > entry->depth_first_order)
					bbg->entry = entry;
			}

			for (k = 0; k < bb->nexts->size; k++) {
				exit      = bb->nexts->data[k];

				if (scf_vector_find(bbg->body, exit))
					continue;

				if (!bbg->exit || bbg->exit->depth_first_order < exit->depth_first_order)
					bbg->exit = exit;
			}

			ret = scf_vector_add(loops, bbg);
			if (ret < 0) {
				scf_bb_group_free(bbg);
				return ret;
			}

			++i;
			++j;
		}
	}

	scf_vector_qsort(loops, _bb_loop_cmp);
	return 0;
}

static int _bb_loop_layers(scf_function_t* f)
{
	int ret;
	int i;
	int j;

	for (i = 0; i < f->bb_loops->size - 1; i++) {

		scf_bb_group_t* loop0 = f->bb_loops->data[i];

		for (j = i + 1; j < f->bb_loops->size; j++) {

			scf_bb_group_t* loop1 = f->bb_loops->data[j];

			int k;
			for (k = 0; k < loop0->body->size; k++) {

				if (!scf_vector_find(loop1->body, loop0->body->data[k]))
					break;
			}

			if (k < loop0->body->size)
				continue;

			if (!loop0->loop_parent
					|| loop0->loop_parent->body->size > loop1->body->size)
				loop0->loop_parent = loop1;

			if (loop1->loop_layers <= loop0->loop_layers + 1)
				loop1->loop_layers = loop0->loop_layers + 1;

			if (!loop1->loop_childs) {
				loop1->loop_childs = scf_vector_alloc();
				if (!loop1->loop_childs)
					return -ENOMEM;
			}

			ret = scf_vector_add_unique(loop1->loop_childs, loop0);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static int _optimize_loop_loads_saves(scf_function_t* f)
{
	scf_bb_group_t*    bbg;
	scf_basic_block_t* bb;
	scf_basic_block_t* jcc_first;
	scf_basic_block_t* jcc_last;
	scf_basic_block_t* pre;
	scf_basic_block_t* post;

	scf_list_t*        sentinel = scf_list_sentinel(&f->basic_block_list_head);

	int i;
	int j;
	int k;

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg = f->bb_loops->data[i];

		if (bbg->loop_layers > 1)
			continue;

		assert(bbg->body->size >= 1);

		assert(sentinel != scf_list_next(&bbg->entry->list));
		assert(sentinel != scf_list_prev(&bbg->exit ->list));

		jcc_first = scf_list_data(scf_list_next(&bbg->entry->list), scf_basic_block_t, list);
		jcc_last  = scf_list_data(scf_list_prev(&bbg->exit ->list), scf_basic_block_t, list);

		assert(jcc_first->jmp_flag);
		assert(jcc_last ->jmp_flag);

		if (!jcc_first->jcc_flag) {
			scf_loge("\n");
			return -EINVAL;
		}
#if 0
		if (!jcc_last->jcc_flag) {
			scf_loge("bbg: %p, entry: %p, exit: %p\n", bbg, bbg->entry, bbg->exit);
			return -EINVAL;
		}
#endif
		pre = scf_basic_block_alloc();
		if (!pre)
			return -ENOMEM;

		post = scf_basic_block_alloc();
		if (!post) {
			scf_basic_block_free(pre);
			return -ENOMEM;
		}

		scf_list_add_front(&jcc_first->list, &pre ->list);
		scf_list_add_front(&jcc_last ->list, &post->list);

		bbg->pre  = pre;
		bbg->post = post;

		pre-> group_flag = 1;
		post->group_flag = 1;

		for (j = 0; j < bbg->body->size; j++) {
			bb = bbg->body->data[j];

			for (k = 0; k < bb->nexts->size; k++) {

				scf_basic_block_t* jcc;
				scf_3ac_code_t*    c;
				scf_list_t*        l;

				if (bb->nexts->data[k] != bbg->exit)
					continue;

				bb->nexts->data[k] = bbg->post;
				assert(0 == scf_vector_del(bbg->exit->prevs, bb));

				l   = scf_list_next(&bb->list);
				assert(l != sentinel);

				jcc = scf_list_data(l, scf_basic_block_t, list);
				assert(jcc->jmp_flag);
				assert(jcc->jcc_flag);

				l = scf_list_head(&jcc->code_list_head);
				c = scf_list_data(l, scf_3ac_code_t, list);
				if (c->dst->bb == bbg->exit)
					c->dst->bb =  bbg->post;
			}

			for (k = 0; k < bb->dn_loads->size; k++) {
				if (scf_vector_add_unique(pre->dn_loads, bb->dn_loads->data[k]) < 0)
					return -1;
			}

			for (k = 0; k < bb->dn_saves->size; k++) {
				if (scf_vector_add_unique(post->dn_saves, bb->dn_saves->data[k]) < 0)
					return -1;
			}

			bb->group_flag = 1;

			scf_vector_clear(bb->dn_loads, NULL);
			scf_vector_clear(bb->dn_saves, NULL);
		}
	}

	return 0;
}

static void _scf_loops_print(scf_vector_t* loops)
{
	int i;
	int j;
	int k;

	for (i = 0; i < loops->size; i++) {
		scf_bb_group_t* loop = loops->data[i];

		printf("loop:  %p\n", loop);
		printf("entry: %p\n", loop->entry);
		printf("exit:  %p\n", loop->exit);
		printf("body: ");
		for (j = 0; j < loop->body->size; j++)
			printf("%p ", loop->body->data[j]);
		printf("\n");

		if (loop->loop_childs) {
			printf("childs: ");
			for (k = 0; k < loop->loop_childs->size; k++)
				printf("%p ", loop->loop_childs->data[k]);
			printf("\n");
		}
		if (loop->loop_parent)
			printf("parent: %p\n", loop->loop_parent);
		printf("loop_layers: %d\n\n", loop->loop_layers);
	}
}

static int _optimize_loop(scf_function_t* f, scf_list_t* bb_list_head)
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

	if (!f->bb_loops) {
		f->bb_loops = scf_vector_alloc();
		if (!f->bb_loops)
			return -ENOMEM;
	} else
		scf_vector_clear(f->bb_loops, ( void (*)(void*) ) scf_vector_free);

	ret = _bb_dfs_loop(bb_list_head, f->bb_loops);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_loop_layers(f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
#if 0
	_scf_loops_print(f->bb_loops);
#endif
	ret = _optimize_loop_loads_saves(f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	return 0;
}

scf_optimizer_t  scf_optimizer_loop =
{
	.name     =  "loop",

	.optimize =  _optimize_loop,
};

