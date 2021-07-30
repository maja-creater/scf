#include"scf_optimizer.h"

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

static int _bb_index_cmp(const void* p0, const void* p1)
{
	const scf_basic_block_t* bb0 = *(scf_basic_block_t**)p0;
	const scf_basic_block_t* bb1 = *(scf_basic_block_t**)p1;

	if (bb0->index < bb1->index)
		return -1;

	if (bb0->index > bb1->index)
		return 1;
	return 0;
}

static int _loop_index_cmp(const void* p0, const void* p1)
{
	scf_bb_group_t*    bbg0 = *(scf_bb_group_t**)p0;
	scf_bb_group_t*    bbg1 = *(scf_bb_group_t**)p1;

	assert(bbg0->body->size > 0);
	assert(bbg1->body->size > 0);

	scf_basic_block_t* bb0  = bbg0->body->data[0];
	scf_basic_block_t* bb1  = bbg1->body->data[0];

	if (bb0->index < bb1->index)
		return -1;

	if (bb0->index > bb1->index)
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

	int nblocks = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		bb->index = nblocks++;

		if (bb->jmp_flag)
			continue;

		int ret;

		int i = 0;
		int j = 0;
		while (i < bb->nexts->size && j < bb->dominators_normal->size) {

			scf_basic_block_t* next = bb->nexts->data[i];
			scf_basic_block_t* dom  = bb->dominators_normal->data[j];

			if (next->dfo_normal < dom->dfo_normal) {
				++i;
				continue;
			}

			if (next->dfo_normal > dom->dfo_normal) {
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

			ret = bbg_find_entry_exit(bbg);
			if (ret < 0) {
				scf_bb_group_free(bbg);
				return ret;
			}
			scf_logd("bbg: %p, entries: %d, exits: %d\n", bbg, bbg->entries->size, bbg->exits->size);

			ret = scf_vector_add(loops, bbg);
			if (ret < 0) {
				scf_bb_group_free(bbg);
				return ret;
			}

			bb->back_flag = 1;

			++i;
			++j;
		}
	}

	scf_vector_qsort(loops, _bb_loop_cmp);
	return 0;
}

static int __bb_loop_merge(scf_vector_t* loops)
{
	scf_basic_block_t* entry;
	scf_basic_block_t* exit;
	scf_basic_block_t* bb;
	scf_bb_group_t*    loop0;
	scf_bb_group_t*    loop1;

	int ret;
	int i;
	int j;

	for (i = 0; i < loops->size - 1; ) {
		loop0     = loops->data[i];

		for (j = i + 1; j < loops->size; j++) {
			loop1         = loops->data[j];

			int k;
			for (k = 0; k < loop0->entries->size; k++) {
				entry     = loop0->entries->data[k];

				if (scf_vector_find(loop1->entries, entry))
					break;
			}

			if (k < loop0->entries->size)
				break;
		}

		if (j == loops->size) {
			i++;
			continue;
		}

		int entries0 = loop0->entries->size;
		int entries1 = loop1->entries->size;
		int exits0   = loop0->exits  ->size;
		int exits1   = loop1->exits  ->size;

		int k;
		for (k = 0; k < loop0->entries->size; ) {
			entry     = loop0->entries->data[k];

			if (scf_vector_find(loop1->body,    entry))
				scf_vector_del (loop0->entries, entry);
			else
				k++;
		}

		for (k = 0; k < loop1->entries->size; ) {
			entry     = loop1->entries->data[k];

			if (scf_vector_find(loop0->body,    entry))
				scf_vector_del (loop1->entries, entry);
			else
				k++;
		}

		for (k = 0; k < loop0->exits->size; ) {
			exit      = loop0->exits->data[k];

			if (scf_vector_find(loop1->body,  exit))
				scf_vector_del (loop0->exits, exit);
			else
				k++;
		}

		for (k = 0; k < loop1->exits->size; ) {
			exit      = loop1->exits->data[k];

			if (scf_vector_find(loop0->body,  exit))
				scf_vector_del (loop1->exits, exit);
			else
				k++;
		}

		assert(loop0->entries->size < entries0);
		assert(loop1->entries->size < entries1);

		assert(loop0->exits->size < exits0);
		assert(loop1->exits->size < exits1);

		if (loop0->exits->size > 0) {
			assert(1 == loop0->exits->size);
			assert(0 == loop1->exits->size);

			for (k = 0; k < loop1->body->size; k++) {
				bb =        loop1->body->data[k];

				assert(0 == scf_vector_add_unique(loop0->body, bb));
			}

			assert(0 == scf_vector_del(loops, loop1));

			scf_bb_group_free(loop1);
			loop1 = NULL;

			assert(0 == bbg_find_entry_exit(loop0));

		} else if (loop1->exits->size > 0) {
			assert(0 == loop0->exits->size);
			assert(1 == loop1->exits->size);

			for (k = 0; k < loop0->body->size; k++) {
				bb =        loop0->body->data[k];

				assert(0 == scf_vector_add_unique(loop1->body, bb));
			}

			assert(0 == scf_vector_del(loops, loop0));

			scf_bb_group_free(loop0);
			loop0 = NULL;

			assert(0 == bbg_find_entry_exit(loop1));
		} else
			assert(0);
	}

	return 0;
}

static int _bb_loop_merge(scf_vector_t* loops)
{
	scf_bb_group_t* loop;

	int ret;
	int i;

	for (i = 0; i < loops->size; i++) {
		loop      = loops->data[i];

		if (loop->loop_childs) {

			ret = _bb_loop_merge(loop->loop_childs);
			if (ret < 0)
				return ret;
		}
	}

	return __bb_loop_merge(loops);
}

static void _bb_loop_sort(scf_vector_t* loops)
{
	scf_bb_group_t* loop;

	int ret;
	int i;

	for (i = 0; i < loops->size; i++) {
		loop      = loops->data[i];

		if (loop->loop_childs)
			_bb_loop_sort(loop->loop_childs);

		scf_vector_qsort(loop->body, _bb_index_cmp);
	}
}

static int __bb_loop_layers(scf_function_t* f)
{
	scf_basic_block_t* entry;
	scf_basic_block_t* exit;
	scf_basic_block_t* bb;
	scf_bb_group_t*    loop0;
	scf_bb_group_t*    loop1;

	int ret;
	int i;
	int j;

	for (i = 0; i < f->bb_loops->size - 1; ) {
		loop0     = f->bb_loops->data[i];

		for (j = i + 1; j < f->bb_loops->size; j++) {
			loop1         = f->bb_loops->data[j];

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
				loop1->loop_layers =  loop0->loop_layers + 1;

			if (!loop1->loop_childs) {
				loop1->loop_childs = scf_vector_alloc();
				if (!loop1->loop_childs)
					return -ENOMEM;
			}

			ret = scf_vector_add_unique(loop1->loop_childs, loop0);
			if (ret < 0)
				return ret;
		}

		if (loop0->loop_parent)
			assert(0 == scf_vector_del(f->bb_loops, loop0));
		else
			i++;
	}

	return 0;
}

static int _bb_loop_layers(scf_function_t* f)
{
	scf_basic_block_t* entry;
	scf_basic_block_t* exit;
	scf_basic_block_t* bb;
	scf_bb_group_t*    loop0;
	scf_bb_group_t*    loop1;

	int ret;
	int i;
	int j;

	ret = __bb_loop_layers(f);
	if (ret < 0)
		return ret;

	ret = _bb_loop_merge(f->bb_loops);
	if (ret < 0)
		return ret;

	_bb_loop_sort(f->bb_loops);


	for (i = 0; i < f->bb_loops->size; i++) {
		loop0     = f->bb_loops->data[i];

		assert(1 == loop0->entries->size);

		loop0->entry = loop0->entries->data[0];
	}

	scf_vector_qsort(f->bb_loops, _loop_index_cmp);
	return 0;
}

static int _bb_loop_add_pre_post(scf_function_t* f)
{
	scf_bb_group_t*    bbg;
	scf_basic_block_t* bb;
	scf_basic_block_t* jmp;
	scf_basic_block_t* exit;
	scf_basic_block_t* pre;
	scf_basic_block_t* post;
	scf_basic_block_t* first;

	scf_list_t*        sentinel = scf_list_sentinel(&f->basic_block_list_head);

	int i;
	int j;
	int k;

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg       = f->bb_loops->data[i];

		assert(bbg->body->size >= 1);

		assert(sentinel != scf_list_next(&bbg->entry->list));

		jmp = scf_list_data(scf_list_next(&bbg->entry->list), scf_basic_block_t, list);

		assert(jmp->jmp_flag);

		if (!jmp->jcc_flag) {
			scf_loge("\n");
			return -EINVAL;
		}

		pre = scf_basic_block_alloc();
		if (!pre)
			return -ENOMEM;
		scf_list_add_front(&jmp->list, &pre->list);
		pre->loop_flag = 1;
		bbg->pre       = pre;

		assert(!bbg->posts);
		bbg->posts = scf_vector_alloc();
		if (!bbg->posts)
			return -ENOMEM;

		for (j = 0; j < bbg->exits->size; j++) {
			exit      = bbg->exits->data[j];

			jmp = scf_list_data(scf_list_prev(&exit->list), scf_basic_block_t, list);

			if (!jmp->jmp_flag) {

				scf_3ac_operand_t* dst;
				scf_3ac_code_t*    c;

				jmp = scf_basic_block_alloc();
				if (!jmp)
					return -ENOMEM;

				c = scf_branch_ops_code(SCF_OP_GOTO, NULL, NULL);
				if (!c) {
					scf_basic_block_free(jmp);
					return -ENOMEM;
				}

				if (scf_vector_add(f->jmps, c) < 0) {
					scf_3ac_code_free(c);
					scf_basic_block_free(jmp);
					return -ENOMEM;
				}

				jmp->jmp_flag  = 1;

				c->basic_block = jmp;

				dst     = c->dsts->data[0];
				dst->bb = exit;

				scf_list_add_tail(&jmp->code_list_head, &c->list);

				scf_list_add_tail(&exit->list, &jmp->list);
			}

			post = scf_basic_block_alloc();
			if (!post)
				return -ENOMEM;

			if (scf_vector_add(bbg->posts, post) < 0) {
				scf_basic_block_free(post);
				return -ENOMEM;
			}

			scf_list_add_front(&jmp->list, &post->list);

			post->loop_flag = 1;
		}

		assert(bbg->exits->size == bbg->posts->size);

		if (bbg->body->size > 0) {

			scf_3ac_operand_t* dst;
			scf_basic_block_t* jcc;
			scf_3ac_code_t*    c;
			scf_list_t*        l;
			scf_list_t*        l2;

			for (j = 0; j < bbg->entry->nexts->size; j++) {
				first     = bbg->entry->nexts->data[j];

				if (scf_vector_find(bbg->body, first))
					break;
			}

			for (j = 0; j < first->prevs->size; j++) {
				bb =        first->prevs->data[j];

				if (scf_vector_find(bbg->body, bb))
					continue;

				for (k = 0; k <  bb->nexts->size; k++) {
					if (first != bb->nexts->data[k])
						continue;

					bb   ->nexts->data[k] = pre;
					first->prevs->data[j] = pre;
//					assert(0 == scf_vector_del(first->prevs, bb));

					if (scf_vector_add_unique(pre->nexts, first) < 0)
						return -ENOMEM;

					for (l  = scf_list_next(&bb->list); l != sentinel; l = scf_list_next(l)) {
						jcc = scf_list_data(l, scf_basic_block_t, list);

						if (!jcc->jmp_flag)
							break;

						l2  = scf_list_head(&jcc->code_list_head);
						c   = scf_list_data(l2, scf_3ac_code_t, list);

						dst = c->dsts->data[0];
						if (dst->bb == first) {
							scf_loge("bb: %p, dst: %p -> %p\n", bb, dst->bb, pre);
							dst->bb =  pre;
						}
					}
				}

				if (scf_vector_add_unique(pre->prevs, bb) < 0)
					return -ENOMEM;
			}
		}

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			for (k = 0; k < bb->nexts->size; k++) {
				exit      = bb->nexts->data[k];

				scf_3ac_operand_t* dst;
				scf_basic_block_t* jcc;
				scf_3ac_code_t*    c;
				scf_list_t*        l;
				scf_list_t*        l2;

				int n;
				for (n = 0; n < bbg->exits->size; n++) {
					if (exit == bbg->exits->data[n])
						break;
				}

				if (n == bbg->exits->size)
					continue;

				post = bbg->posts->data[n];

				for (n = 0; n < exit->prevs->size; n++) {
					if (bb   == exit->prevs->data[n])
						break;
				}
				assert(n < exit->prevs->size);

				bb  ->nexts->data[k] = post;

				if (!scf_vector_find(exit->prevs, post))
					exit->prevs->data[n] = post;
				else
					assert(0 == scf_vector_del(exit->prevs, bb));

				if (scf_vector_add_unique(post->prevs, bb) < 0)
					return -ENOMEM;

				if (scf_vector_add_unique(post->nexts, exit) < 0)
					return -ENOMEM;

				for (l  = scf_list_next(&bb->list); l != sentinel; l = scf_list_next(l)) {

					jcc = scf_list_data(l, scf_basic_block_t, list);
					if (!jcc->jmp_flag)
						break;

					l2  = scf_list_head(&jcc->code_list_head);
					c   = scf_list_data(l2, scf_3ac_code_t, list);
					dst = c->dsts->data[0];
					if (dst->bb == exit)
						dst->bb =  post;
				}
			}

			bb->loop_flag = 1;
		}
	}

	return 0;
}

static int _optimize_loop_loads_saves(scf_function_t* f)
{
	scf_bb_group_t*    bbg;
	scf_basic_block_t* bb;
	scf_basic_block_t* pre;
	scf_basic_block_t* post;
	scf_dag_node_t*    dn;

	int i;
	int j;
	int k;

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg       = f->bb_loops->data[i];

		assert(bbg->body->size >= 1);

		pre  = bbg->pre;

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			for (k = 0; k < bb->dn_loads->size; k++) {
				dn =        bb->dn_loads->data[k];

				if (dn->var->tmp_flag) {
					k++;
					continue;
				}

				if (scf_vector_add_unique(pre->dn_loads, dn) < 0)
					return -1;
			}

			for (k = 0; k < bbg->posts->size; k++) {
				post      = bbg->posts->data[k];

				int n;
				for (n = 0; n < bb->dn_saves->size; n++) {
					dn =        bb->dn_saves->data[n];

					if (scf_vector_add_unique(post->dn_saves, dn) < 0)
						return -1;
				}
			}
		}
	}

	return 0;
}

static int _bb_not_in_loop(scf_function_t* f, scf_list_t* bb_list_head)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;
	scf_bb_group_t*    bbg;
	scf_bb_group_t*    bbg2;

	int start = 0;
	int i;
	int j;

	bbg = NULL;
	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		if (bb->jmp_flag || bb->end_flag)
			continue;

		for (i   = start; i < f->bb_loops->size; i++) {
			bbg2 =            f->bb_loops->data[i];

			for (j  = 0; j < bbg2->body->size; j++) {
				bb2 =        bbg2->body->data[j];

				if (bb == bb2)
					break;
			}
			if (j < bbg2->body->size)
				break;
		}

		if (i < f->bb_loops->size) {
			if (bbg) {
				if (scf_vector_add(f->bb_groups, bbg) < 0) {
					scf_bb_group_free(bbg);
					return -ENOMEM;
				}

				bbg   = NULL;
				start = i;
			}
			continue;
		}

		if (!bbg) {
			bbg = scf_bb_group_alloc();
			if (!bbg)
				return -ENOMEM;

		} else if (bbg->body->size > 0) {

			for (i = 0; i < bb->prevs->size; i++) {
				bb2       = bb->prevs->data[i];

				if (scf_vector_find(bbg->body, bb2))
					break;
			}

			if (i == bb->prevs->size) {

				if (scf_vector_add(f->bb_groups, bbg) < 0) {
					scf_bb_group_free(bbg);
					return -ENOMEM;
				}

				bbg = scf_bb_group_alloc();
				if (!bbg)
					return -ENOMEM;
			}
		}

		if (scf_vector_add(bbg->body, bb) < 0) {
			scf_bb_group_free(bbg);
			return -ENOMEM;
		}
	}

	if (bbg) {
		if (scf_vector_add(f->bb_groups, bbg) < 0) {
			scf_bb_group_free(bbg);
			return -ENOMEM;
		}
	}

	return 0;
}

static int _optimize_loop(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	if (!f->bb_loops) {
		f->bb_loops = scf_vector_alloc();
		if (!f->bb_loops)
			return -ENOMEM;
	} else
		scf_vector_clear(f->bb_loops, ( void (*)(void*) ) scf_vector_free);

	if (!f->bb_groups) {
		f ->bb_groups= scf_vector_alloc();
		if (!f->bb_groups)
			return -ENOMEM;
	} else
		scf_vector_clear(f->bb_groups, ( void (*)(void*) ) scf_vector_free);

	int ret = _bb_dfs_loop(bb_list_head, f->bb_loops);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_loop_layers(f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_not_in_loop(f, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_loop_add_pre_post(f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

#if 1
	ret = _optimize_loop_loads_saves(f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
#endif
	//scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_loop =
{
	.name     =  "loop",

	.optimize =  _optimize_loop,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

