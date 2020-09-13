#include"scf_basic_block.h"
#include"scf_dag.h"
#include"scf_3ac.h"

scf_basic_block_t* scf_basic_block_alloc()
{
	scf_basic_block_t* bb = calloc(1, sizeof(scf_basic_block_t));
	if (!bb)
		return NULL;

	scf_list_init(&bb->list);
	scf_list_init(&bb->dag_list_head);
	scf_list_init(&bb->code_list_head);

	bb->prevs = scf_vector_alloc();
	if (!bb->prevs)
		goto error_prevs;

	bb->nexts = scf_vector_alloc();
	if (!bb->nexts)
		goto error_nexts;

	bb->entry_dn_actives = scf_vector_alloc();
	if (!bb->entry_dn_actives)
		goto error_entry;

	bb->exit_dn_actives  = scf_vector_alloc();
	if (!bb->exit_dn_actives)
		goto error_exit;

	bb->dn_updateds = scf_vector_alloc();
	if (!bb->dn_updateds)
		goto error_updateds;

	bb->dn_loads = scf_vector_alloc();
	if (!bb->dn_loads)
		goto error_loads;

	bb->dn_saves = scf_vector_alloc();
	if (!bb->dn_saves)
		goto error_saves;

	bb->dn_colors = scf_vector_alloc();
	if (!bb->dn_colors)
		goto error_colors;

	bb->dn_status_initeds = scf_vector_alloc();
	if (!bb->dn_status_initeds)
		goto error_dn_status_initeds;

	bb->dn_pointer_aliases = scf_vector_alloc();
	if (!bb->dn_pointer_aliases)
		goto error_dn_pointer_aliases;

	bb->entry_dn_aliases = scf_vector_alloc();
	if (!bb->entry_dn_aliases)
		goto error_entry_dn_aliases;

	bb->exit_dn_aliases = scf_vector_alloc();
	if (!bb->exit_dn_aliases)
		goto error_exit_dn_aliases;

	bb->dn_reloads = scf_vector_alloc();
	if (!bb->dn_reloads)
		goto error_reloads;

	bb->dn_resaves = scf_vector_alloc();
	if (!bb->dn_resaves)
		goto error_resaves;

	return bb;

error_resaves:
	scf_vector_free(bb->dn_reloads);
error_reloads:
	scf_vector_free(bb->exit_dn_aliases);
error_exit_dn_aliases:
	scf_vector_free(bb->entry_dn_aliases);
error_entry_dn_aliases:
	scf_vector_free(bb->dn_pointer_aliases);
error_dn_pointer_aliases:
	scf_vector_free(bb->dn_status_initeds);
error_dn_status_initeds:
	scf_vector_free(bb->dn_colors);
error_colors:
	scf_vector_free(bb->dn_saves);
error_saves:
	scf_vector_free(bb->dn_loads);
error_loads:
	scf_vector_free(bb->dn_updateds);
error_updateds:
	scf_vector_free(bb->exit_dn_actives);
error_exit:
	scf_vector_free(bb->entry_dn_actives);
error_entry:
	scf_vector_free(bb->nexts);
error_nexts:
	scf_vector_free(bb->prevs);
error_prevs:
	free(bb);
	return NULL;
}

void scf_basic_block_free(scf_basic_block_t* bb)
{
	if (bb) {
		// this bb's DAG nodes freed here
		scf_list_clear(&bb->dag_list_head, scf_dag_node_t, list, scf_dag_node_free);

		scf_list_clear(&bb->code_list_head, scf_3ac_code_t, list, scf_3ac_code_free);

		// DAG nodes were freed by 'scf_list_clear' above, so only free this vector
		if (bb->var_dag_nodes)
			scf_vector_free(bb->var_dag_nodes);

		if (bb->prevs)
			scf_vector_free(bb->prevs);

		if (bb->nexts)
			scf_vector_free(bb->nexts);
	}
}

void scf_bb_group_free(scf_bb_group_t* bbg)
{
	if (bbg) {
		if (bbg->body)
			scf_vector_free(bbg->body);

		free(bbg);
	}
}

int scf_basic_block_connect(scf_basic_block_t* prev_bb, scf_basic_block_t* next_bb)
{
	int ret = scf_vector_add_unique(prev_bb->nexts, next_bb);
	if (ret < 0)
		return ret;

	return scf_vector_add_unique(next_bb->prevs, prev_bb);
}

void scf_basic_block_print(scf_basic_block_t* bb, scf_list_t* sentinel)
{
	if (bb) {
#define SCF_BB_PRINT(h) \
		do { \
			scf_list_t* l; \
			for (l = scf_list_head(&bb->h); l != scf_list_sentinel(&bb->h); \
					l = scf_list_next(l)) { \
				scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list); \
				scf_3ac_code_print(c, sentinel); \
			} \
		} while (0)

		SCF_BB_PRINT(code_list_head);
	}
}

void scf_basic_block_print_list(scf_list_t* h)
{
	scf_list_t* l;

	if (!scf_list_empty(h)) {

		scf_basic_block_t* last_bb  = scf_list_data(scf_list_tail(h), scf_basic_block_t, list);
		scf_list_t*        sentinel = scf_list_sentinel(&last_bb->code_list_head);

		for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

			scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

			printf("\033[33mbasic_block: %p, index: %d\033[0m\n", bb, bb->index);
			scf_basic_block_print(bb, sentinel);

			printf("\n");
#if 1
			int i;
			if (bb->prevs) {
				for (i = 0; i < bb->prevs->size; i++)
					printf("prev     : %p\n", bb->prevs->data[i]);
			}
			if (bb->nexts) {
				for (i = 0; i < bb->nexts->size; i++)
					printf("next     : %p\n", bb->nexts->data[i]);
			}
			if (bb->dominators) {
				for (i = 0; i < bb->dominators->size; i++)
					printf("dominator: %p\n", bb->dominators->data[i]);
			}
			if (bb->dn_status_initeds) {
				for (i = 0; i < bb->dn_status_initeds->size; i++) {

					scf_active_var_t* av = bb->dn_status_initeds->data[i];
					scf_dag_node_t*   dn = av->dag_node;
					scf_variable_t*   v  = dn->var;

					if (v && v->w)
						printf("inited: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}
			printf("\n");
#endif
		}
	}
}

static int _copy_to_active_vars(scf_vector_t* active_vars, scf_vector_t* dag_nodes)
{
	scf_dag_node_t*   dn;
	scf_active_var_t* status;

	int i;

	for (i = 0; i < dag_nodes->size; i++) {

		dn     = dag_nodes->data[i];

		status = scf_vector_find_cmp(active_vars, dn, scf_dn_status_cmp);

		if (!status) {
			status = scf_active_var_alloc(dn);
			if (!status)
				return -ENOMEM;

			int ret = scf_vector_add(active_vars, status);
			if (ret < 0) {
				scf_active_var_free(status);
				return ret;
			}
		}
#if 0
		status->alias   = dn->alias;
		status->value   = dn->value;
#endif
		status->active  = dn->active;
		status->inited  = dn->inited;
		status->updated = dn->updated;

		dn->inited      = 0;
	}

	return 0;
}

static int _copy_from_active_vars(scf_vector_t* dag_nodes, scf_vector_t* active_vars)
{
	if (!dag_nodes)
		return -EINVAL;

	if (!active_vars)
		return 0;

	int j;
	for (j = 0; j < active_vars->size; j++) {
		scf_active_var_t* v  = active_vars->data[j];
		scf_dag_node_t*   dn = v->dag_node;

		if (scf_variable_const(dn->var))
			continue;

		if (!v->active)
			continue;

		if (scf_type_is_var(dn->type) && (dn->var->global_flag || dn->var->local_flag)) {

			int ret = scf_vector_add_unique(dag_nodes, dn);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static int _copy_updated_vars(scf_vector_t* dag_nodes, scf_vector_t* active_vars)
{
	if (!dag_nodes)
		return -EINVAL;

	if (!active_vars)
		return 0;

	int j;
	for (j = 0; j < active_vars->size; j++) {
		scf_active_var_t* v  = active_vars->data[j];
		scf_dag_node_t*   dn = v->dag_node;

		if (scf_variable_const(dn->var))
			continue;

		if (!v->updated)
			continue;

		if (scf_type_is_var(dn->type) && (dn->var->global_flag || dn->var->local_flag)) {

			int ret = scf_vector_add_unique(dag_nodes, dn);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static int _bb_vars(scf_basic_block_t* bb)
{
	int ret = 0;

	scf_list_t*     l;
	scf_dag_node_t* dn;

	if (!bb->var_dag_nodes) {
		bb->var_dag_nodes = scf_vector_alloc();
		if (!bb->var_dag_nodes)
			return -ENOMEM;
	} else
		scf_vector_clear(bb->var_dag_nodes, NULL);

	for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_prev(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (scf_type_is_jmp(c->op->type))
			continue;

		if (c->dst && c->dst->dag_node) {
			dn     =  c->dst->dag_node;

			if (scf_type_is_var(dn->type)
					&& (dn->var->global_flag || dn->var->local_flag)
					&& !scf_variable_const(dn->var)) {

				ret = scf_vector_add_unique(bb->var_dag_nodes, dn);
				if (ret < 0)
					return ret;
			}
		}

		if (c->srcs) {
			int j;
			for (j = 0; j < c->srcs->size; j++) {
				scf_3ac_operand_t* src = c->srcs->data[j];

				if (!src->node)
					continue;

				assert(src->dag_node);
				dn = src->dag_node;

				if (scf_type_is_operator(dn->type)
						|| !scf_variable_const(dn->var)) {

					ret = scf_vector_add_unique(bb->var_dag_nodes, dn);
					if (ret < 0)
						return ret;
				}
			}
		}
	}

	return 0;
}

static int _bb_dag(scf_basic_block_t* bb)
{
	int ret = 0;
	int i;
	int j;

	scf_list_t* l;
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		ret = scf_3ac_code_to_dag(c, &bb->dag_list_head);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int _bb_update_dag(scf_basic_block_t* bb, scf_list_t* dag_list_head)
{
	int ret = 0;
	int i;
	scf_list_t* l;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		ret = scf_3ac_code_to_dag(c, dag_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
		printf("\n");
	}

	for (i = 0; i < bb->var_dag_nodes->size; i++) {
		scf_dag_node_t* dn0 = bb->var_dag_nodes->data[i];

		scf_dag_node_t* dn1 = scf_dag_find_dn(dag_list_head, dn0);
		if (!dn1) {
			scf_loge("dn0: v_%d_%d/%s\n", dn0->var->w->line, dn0->var->w->pos, dn0->var->w->text->data);
			return -EINVAL;
		}

		bb->var_dag_nodes->data[i] = dn1;
	}

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (!c->active_vars)
			continue;

		for (i = 0; i < c->active_vars->size; i++) {
			scf_active_var_t* v   = c->active_vars->data[i];
			scf_dag_node_t*   dn0 = v->dag_node;

			scf_dag_node_t*   dn1 = scf_dag_find_dn(dag_list_head, dn0);
			if (!dn1) {
				scf_loge("\n");
				return -EINVAL;
			}

			v->dag_node = dn1;
		}
	}

//	scf_dag_node_free_list(&bb->dag_list_head);
	return 0;
}

int scf_basic_block_dag(scf_basic_block_t* bb, scf_list_t* dag_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	int ret;

	ret = _bb_dag(bb);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_vars(bb);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (c->active_vars)
			scf_vector_clear(c->active_vars, (void (*)(void*))scf_active_var_free);
		else {
			c->active_vars = scf_vector_alloc();
			if (!c->active_vars)
				return -ENOMEM;
		}

		ret = _copy_to_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			return ret;
	}

	ret = _bb_update_dag(bb, dag_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return scf_basic_block_inited_vars(bb);
}

int scf_basic_block_inited_vars(scf_basic_block_t* bb)
{
	int ret = 0;
	int i;

	scf_list_t*       l;
	scf_3ac_code_t*   c;
	scf_dag_node_t*   dn;
	scf_active_var_t* status;
	scf_active_var_t* status2;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (!c->active_vars)
			continue;

		if (c->dst && c->dst->dag_node && SCF_OP_ASSIGN == c->op->type) {

			dn = c->dst->dag_node;

			if (scf_type_is_var(dn->type)
					&& (dn->var->global_flag || dn->var->local_flag)) {

#define SCF_DN_STATUS_GET(ds, vec, dn) \
				do { \
					ds = scf_vector_find_cmp(vec, dn, scf_dn_status_cmp); \
					if (!ds) { \
						ds = scf_active_var_alloc(dn); \
						if (!ds) \
							return -ENOMEM; \
						int ret = scf_vector_add(vec, ds); \
						if (ret < 0) { \
							scf_active_var_free(ds); \
							return ret; \
						} \
					} \
				} while (0)

				SCF_DN_STATUS_GET(status, c->active_vars, dn);
				dn->inited     = 1;
				status->inited = 1;

				SCF_DN_STATUS_GET(status2, bb->dn_status_initeds, dn);
				status2->inited = 1;

				if (dn->var->nb_pointers > 0) {
					scf_dag_pointer_alias(status, dn, c);

					dn->alias      = status->alias;
					status2->alias = status->alias;
				}
			}
		}

		for (i = 0; i < bb->dn_status_initeds->size; i++) {
			status2 = bb->dn_status_initeds->data[i];
			dn      = status2->dag_node;

			SCF_DN_STATUS_GET(status, c->active_vars, dn);

			assert(status);
			status->alias = status2->alias;
		}
	}

	return 0;
}

int scf_basic_block_active_vars(scf_basic_block_t* bb)
{
	scf_list_t*       l;
	scf_3ac_code_t*   c;
	scf_dag_node_t*   dn;
	scf_active_var_t* status;

	int i;
	int j;
	int ret;

	ret = _bb_vars(bb);
	if (ret < 0)
		return ret;

	for (i = 0; i < bb->var_dag_nodes->size; i++) {

		dn = bb->var_dag_nodes->data[i];

		if (scf_type_is_var(dn->type)
				&& (dn->var->global_flag || dn->var->local_flag)
				&& !scf_variable_const(dn->var))
			dn->active  = 1;
		else
			dn->active  = 0;

		dn->updated = 0;
	}

	for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_prev(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (scf_type_is_jmp(c->op->type))
			continue;

		if (c->dst && c->dst->dag_node) {

			if (scf_type_is_binary_assign(c->op->type))
				c->dst->dag_node->active = 1;
			else
				c->dst->dag_node->active = 0;

			c->dst->dag_node->updated = 1;
		}

		if (c->srcs) {
			for (j = 0; j < c->srcs->size; j++) {
				scf_3ac_operand_t* src = c->srcs->data[j];

				if (!src->node)
					continue;

				assert(src->dag_node);

				if (SCF_OP_ADDRESS_OF == c->op->type
						&& scf_type_is_var(src->dag_node->type))
					src->dag_node->active = 0;
				else
					src->dag_node->active = 1;

				if (SCF_OP_INC == c->op->type
						|| SCF_OP_INC_POST == c->op->type
						|| SCF_OP_DEC      == c->op->type
						|| SCF_OP_DEC_POST == c->op->type)
					src->dag_node->updated = 1;
			}
		}

		if (c->active_vars)
			scf_vector_clear(c->active_vars, (void (*)(void*))scf_active_var_free);
		else {
			c->active_vars = scf_vector_alloc();
			if (!c->active_vars)
				return -ENOMEM;
		}

		ret = _copy_to_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			return ret;
	}

	if (!scf_list_empty(&bb->code_list_head)) {

		l = scf_list_head(&bb->code_list_head);
		c = scf_list_data(l, scf_3ac_code_t, list);

		ret = _copy_from_active_vars(bb->entry_dn_actives, c->active_vars);
		if (ret < 0)
			return ret;

		ret = _copy_updated_vars(bb->dn_updateds, c->active_vars);
		if (ret < 0)
			return ret;
	}

	return 0;
}

