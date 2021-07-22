#include"scf_optimizer.h"

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

static int _optimize_generate_loads_saves(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_basic_block_t* post;
	scf_bb_group_t*    bbg;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    load;
	scf_3ac_code_t*    save;
	scf_3ac_code_t*    c;

	int i;
	int ret;

#define SCF_OPTIMIZER_LOAD(load_type, h) \
			do { \
				load = scf_3ac_alloc_by_dst(load_type, dn); \
				if (!load) { \
					scf_loge("\n"); \
					return -ENOMEM; \
				} \
				load->basic_block = bb; \
				scf_list_add_front(h, &load->list); \
			} while (0)

#define SCF_OPTIMIZER_SAVE(save_type, h) \
			do { \
				save = scf_3ac_alloc_by_src(save_type, dn); \
				if (!save) { \
					scf_loge("\n"); \
					return -ENOMEM; \
				} \
				save->basic_block = bb; \
				scf_list_add_tail(h, &save->list); \
			} while (0)

	f->nb_basic_blocks = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

		bb        = scf_list_data(l, scf_basic_block_t, list);

		bb->index = f->nb_basic_blocks++;

		if (bb->generate_flag) {
			for (i = 0; i < bb->dn_loads->size; i++) {
				dn =        bb->dn_loads->data[i];

				if (scf_vector_find(bb->dn_reloads, dn))
					continue;

				SCF_OPTIMIZER_LOAD(SCF_OP_3AC_LOAD, &bb->code_list_head);
			}

			for (i = 0; i < bb->dn_saves->size; i++) {
				dn =        bb->dn_saves->data[i];

				if (scf_vector_find(bb->dn_resaves, dn))
					continue;

				if (bb->loop_flag)
					SCF_OPTIMIZER_SAVE(SCF_OP_3AC_SAVE, &bb->save_list_head);
				else
					SCF_OPTIMIZER_SAVE(SCF_OP_3AC_SAVE, &bb->code_list_head);
			}
		}

		for (i = 0; i < bb->dn_reloads->size; i++) {
			dn =        bb->dn_reloads->data[i];
			SCF_OPTIMIZER_LOAD(SCF_OP_3AC_RELOAD, &bb->code_list_head);
		}

		for (i = 0; i < bb->dn_resaves->size; i++) {
			dn =        bb->dn_resaves->data[i];
			SCF_OPTIMIZER_SAVE(SCF_OP_3AC_RESAVE, &bb->code_list_head);
		}

		if (bb->auto_ref_flag || bb->auto_free_flag) {

			scf_3ac_code_t* c0 = scf_3ac_code_alloc();
			scf_3ac_code_t* c1 = scf_3ac_code_alloc();

			c0->op = scf_3ac_find_operator(SCF_OP_3AC_PUSH_RAX);
			c1->op = scf_3ac_find_operator(SCF_OP_3AC_POP_RAX);

			scf_list_add_front(&bb->code_list_head, &c0->list);
			scf_list_add_tail(&bb->code_list_head,  &c1->list);
		}
#if 1
		scf_logd("bb: %p, bb->index: %d\n", bb, bb->index);
		ret = scf_basic_block_active_vars(bb);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
#endif
	}

	for (i = 0; i < f->bb_groups->size; i++) {
		bbg       = f->bb_groups->data[i];

		scf_dn_status_t* ds;
		int j;
		int k;
		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			for (k = 0; k < bb->exit_dn_actives->size; k++) {
				dn =        bb->exit_dn_actives->data[k];

				for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); l = scf_list_prev(l)) {
					c  = scf_list_data(l, scf_3ac_code_t, list);
					assert(c->active_vars);

					ds = scf_vector_find_cmp(c->active_vars, dn, scf_dn_status_cmp);
					if (ds)
						break;

					SCF_DN_STATUS_ALLOC(ds, c->active_vars, dn);
					ds->active = 1;
				}
			}

			for (k = 0; k < bb->exit_dn_aliases->size; k++) {
				dn =        bb->exit_dn_aliases->data[k];

				for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); l = scf_list_prev(l)) {
					c  = scf_list_data(l, scf_3ac_code_t, list);
					assert(c->active_vars);

					ds = scf_vector_find_cmp(c->active_vars, dn, scf_dn_status_cmp);
					if (ds)
						break;

					SCF_DN_STATUS_ALLOC(ds, c->active_vars, dn);
					ds->active = 1;
				}
			}
		}
	}

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg = f->bb_loops->data[i];

		qsort(bbg->body->data, bbg->body->size, sizeof(void*), _bb_index_cmp);

		scf_dn_status_t* ds;
		int j;
		int k;
		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); l = scf_list_prev(l)) {
				c  = scf_list_data(l, scf_3ac_code_t, list);
				assert(c->active_vars);

				int n;
				for (n = 0; n < bbg->posts->size; n++) {
					post      = bbg->posts->data[n];

					for (k  = 0; k < post->dn_saves->size; k++) {
						dn  =        post->dn_saves->data[k];

						SCF_DN_STATUS_GET(ds, c->active_vars, dn);
						ds->active = 1;
					}
				}

				for (k  = 0; k < bbg->pre->dn_loads->size; k++) {
					dn  =        bbg->pre->dn_loads->data[k];

					SCF_DN_STATUS_GET(ds, c->active_vars, dn);
					ds->active = 1;
				}
			}
		}
	}

	for (i = 0; i < f->jmps->size; i++) {
		c  =        f->jmps->data[i];

		scf_3ac_operand_t* dst = c->dsts->data[0];

		dst->bb->jmp_dst_flag = 1;
	}

	return 0;
}

scf_optimizer_t  scf_optimizer_generate_loads_saves =
{
	.name     =  "generate_loads_saves",

	.optimize =  _optimize_generate_loads_saves,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

