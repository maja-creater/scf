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

	return bb;

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

			printf("\033[33mbasic_block: %p\033[0m\n", bb);
			scf_basic_block_print(bb, sentinel);

			printf("\n");

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
			printf("\n");
		}
	}
}

static int _copy_to_active_vars(scf_vector_t* active_vars, scf_vector_t* dag_nodes)
{
	int i;
	for (i = 0; i < dag_nodes->size; i++) {
		scf_active_var_t* v = scf_active_var_alloc(dag_nodes->data[i]);
		if (!v)
			return -ENOMEM;

		int ret = scf_vector_add(active_vars, v);
		if (ret < 0) {
			scf_active_var_free(v);
			v = NULL;
			return ret;
		}
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

static int _bb_active_vars(scf_basic_block_t* bb)
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
		printf("\n");
	}

	scf_vector_t* roots = scf_vector_alloc();
	if (!roots)
		return -ENOMEM;

	scf_vector_t* leafs = scf_vector_alloc();
	if (!leafs) {
		scf_vector_free(roots);
		return -ENOMEM;
	}

	if (!bb->var_dag_nodes) {
		bb->var_dag_nodes = scf_vector_alloc();
		if (!bb->var_dag_nodes) {
			ret = -ENOMEM;
			goto error;
		}
	} else {
		scf_vector_clear(bb->var_dag_nodes, NULL);
	}

	ret = scf_dag_find_roots(&bb->dag_list_head, roots);
	if (ret < 0)
		goto error;

	ret = scf_dag_vector_find_leafs(roots, leafs);
	if (ret < 0)
		goto error;

	for (i = 0; i < leafs->size; i++) {
		scf_dag_node_t* dn = leafs->data[i];

		// it should be 'real' variable, not 'const' variable,
		// const pointer var can be changed, but what it points to can't be changed.
		if (!scf_variable_const(dn->var)) {
			dn->active = 1;

			ret = scf_vector_add(bb->var_dag_nodes, dn);
			if (ret < 0)
				goto error;

			// load global or local var
			if (scf_type_is_var(dn->type) && (dn->var->global_flag || dn->var->local_flag)) {
				scf_logw("type: %d, var: %s, const_flag: %d, const_literal_flag: %d, global_flag: %d, local_flag: %d, nb_pointers: %d\n",
						dn->type,
						dn->var->w->text->data,
						dn->var->const_flag,
						dn->var->const_literal_flag,
						dn->var->global_flag,
						dn->var->local_flag,
						dn->var->nb_pointers);
			}
		}
	}

	for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_prev(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

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
			int j;
			for (j = 0; j < c->srcs->size; j++) {
				scf_3ac_operand_t* src = c->srcs->data[j];

				if (!src->node)
					continue;

				assert(src->dag_node);
				src->dag_node->active = 1;

				if (SCF_OP_INC == c->op->type
						|| SCF_OP_INC_POST == c->op->type
						|| SCF_OP_DEC      == c->op->type
						|| SCF_OP_DEC_POST == c->op->type)
					src->dag_node->updated = 1;

				if (scf_type_is_operator(src->dag_node->type)
						|| !scf_variable_const(src->dag_node->var)) {

					ret = scf_vector_add_unique(bb->var_dag_nodes, src->dag_node);
					if (ret < 0)
						goto error;
				}
			}
		}

		if (c->active_vars) {
			scf_vector_clear(c->active_vars, (void (*)(void*))scf_active_var_free);
		} else {
			c->active_vars = scf_vector_alloc();
			if (!c->active_vars) {
				ret = -ENOMEM;
				goto error;

			}
		}

		ret = _copy_to_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			goto error;
	}

	return 0;

error:
	scf_vector_free(leafs);
	scf_vector_free(roots);
	return ret;
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

	scf_dag_node_free_list(&bb->dag_list_head);
	return 0;
}

int scf_basic_block_active_vars(scf_basic_block_t* bb, scf_list_t* dag_list_head)
{
	int ret = 0;
	int i;
	int j;

	scf_list_t* l;

	ret = _bb_active_vars(bb);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = _bb_update_dag(bb, dag_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (!scf_list_empty(&bb->code_list_head)) {
		scf_3ac_code_t* c;
#if 1
		l   = scf_list_head(&bb->code_list_head);
		c   = scf_list_data(l, scf_3ac_code_t, list);

		ret = _copy_from_active_vars(bb->entry_dn_actives, c->active_vars);
		if (ret < 0)
			goto error;

		ret = _copy_updated_vars(bb->dn_updateds, c->active_vars);
		if (ret < 0)
			goto error;
#endif
	}
#if 0
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_print(c, NULL);

		if (c->active_vars) {
			for (j = 0; j < c->active_vars->size; j++) {
				scf_active_var_t* v = c->active_vars->data[j];

				if (!v->dag_node->var)
					continue;

				if (v->dag_node->var->w)
					scf_logw("dn: %p, var: %s, active: %d\n", v->dag_node, v->dag_node->var->w->text->data, v->active);
				else
					scf_logw("dn: %p, var: %p, active: %d\n", v->dag_node, v->dag_node->var, v->active);
			}
		}
		printf("\n");
	}
#endif

	return 0;

error:
	return ret;
}

