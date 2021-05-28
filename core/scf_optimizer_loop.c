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
			scf_basic_block_t* bb2;
			int k;

			for (k = 0; k < dom->prevs->size; k++) {
				entry     = dom->prevs->data[k];

				if (scf_vector_find(bbg->body, entry))
					continue;

				if (!bbg->entry || bbg->entry->depth_first_order > entry->depth_first_order)
					bbg->entry = entry;
			}

			int n;
			for (n  = 0; n < bbg->body->size; n++) {
				bb2 =        bbg->body->data[n];

				for (k = 0; k < bb2->nexts->size; k++) {
					exit      = bb2->nexts->data[k];

					if (scf_vector_find(bbg->body, exit))
						continue;

					if (!bbg->exit || bbg->exit->depth_first_order < exit->depth_first_order)
						bbg->exit = exit;
				}
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

	for (i = 0; i < f->bb_loops->size - 1; ) {

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

static int _optimize_peep_hole(scf_bb_group_t* bbg, scf_basic_block_t* bb)
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
	}
	return 0;
}

static int _bb_loop_add_pre_post(scf_function_t* f)
{
	scf_bb_group_t*    bbg;
	scf_basic_block_t* bb;
	scf_basic_block_t* jcc_first;
	scf_basic_block_t* jcc_last;
	scf_basic_block_t* pre;
	scf_basic_block_t* post;
	scf_basic_block_t* first;

	scf_list_t*        sentinel = scf_list_sentinel(&f->basic_block_list_head);

	int i;
	int j;
	int k;

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg = f->bb_loops->data[i];

//		if (bbg->loop_layers > 1)
//			continue;

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

		scf_logd("bbg: %p, entry: %p, exit: %p\n", bbg, bbg->entry, bbg->exit);

		if (bbg->body->size > 0) {

			scf_3ac_operand_t* dst;
			scf_basic_block_t* jcc;
			scf_3ac_code_t*    c;
			scf_list_t*        l;

			for (j = 0; j < bbg->entry->nexts->size; j++) {
				first     = bbg->entry->nexts->data[j];

				if (scf_vector_find(bbg->body, first))
					break;
			}

			for (j = 0; j < first->prevs->size; j++) {
				bb = first->prevs->data[j];

				if (scf_vector_find(bbg->body, bb))
					continue;

				for (k = 0; k <  bb->nexts->size; k++) {
					if (first != bb->nexts->data[k])
						continue;

					bb->nexts->data[k] = pre;
					assert(0 == scf_vector_del(first->prevs, bb));

					for (l  = scf_list_next(&bb->list); l != sentinel; l = scf_list_next(l)) {
						jcc = scf_list_data(l, scf_basic_block_t, list);

						if (!jcc->jmp_flag)
							break;

						l   = scf_list_head(&jcc->code_list_head);
						c   = scf_list_data(l, scf_3ac_code_t, list);
						dst = c->dsts->data[0];
						if (dst->bb == first)
							dst->bb =  pre;
					}
				}
			}
		}

		for (j = 0; j < bbg->body->size; j++) {
			bb = bbg->body->data[j];

			for (k = 0; k < bb->nexts->size; k++) {

				scf_3ac_operand_t* dst;
				scf_basic_block_t* jcc;
				scf_3ac_code_t*    c;
				scf_list_t*        l;
				scf_list_t*        l2;

				if (bb->nexts->data[k] != bbg->exit)
					continue;

				bb->nexts->data[k] = bbg->post;
				assert(0 == scf_vector_del(bbg->exit->prevs, bb));

				for (l  = scf_list_next(&bb->list); l != sentinel; l = scf_list_next(l)) {

					jcc = scf_list_data(l, scf_basic_block_t, list);
					if (!jcc->jmp_flag)
						break;

					l2  = scf_list_head(&jcc->code_list_head);
					c   = scf_list_data(l2, scf_3ac_code_t, list);
					dst = c->dsts->data[0];
					if (dst->bb == bbg->exit)
						dst->bb =  bbg->post;
				}
			}

			bb->group_flag = 1;
		}

		assert(0 == scf_vector_add_unique(bbg->exit->prevs, bbg->post));
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
		bbg = f->bb_loops->data[i];

//		if (bbg->loop_layers > 1)
//			continue;

		assert(bbg->body->size >= 1);

		pre  = bbg->pre;
		post = bbg->post;

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			int ret = _optimize_peep_hole(bbg, bb);
			if (ret < 0)
				return ret;
		}

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			for (k = 0; k < bb->dn_loads->size; ) {
				dn =        bb->dn_loads->data[k];

				if (dn->var->tmp_flag) {
					k++;
					continue;
				}

				if (scf_vector_add_unique(pre->dn_loads, dn) < 0)
					return -1;

				assert(0 == scf_vector_del(bb->dn_loads, dn));
			}

			for (k = 0; k < bb->dn_saves->size; k++) {
				if (scf_vector_add_unique(post->dn_saves, bb->dn_saves->data[k]) < 0)
					return -1;
			}
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

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int ret;
	int i;

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
	return 0;
}

scf_optimizer_t  scf_optimizer_loop =
{
	.name     =  "loop",

	.optimize =  _optimize_loop,
};

