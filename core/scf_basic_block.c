#include"scf_basic_block.h"
#include"scf_dag.h"
#include"scf_3ac.h"
#include"scf_pointer_alias.h"

scf_basic_block_t* scf_basic_block_alloc()
{
	scf_basic_block_t* bb = calloc(1, sizeof(scf_basic_block_t));
	if (!bb)
		return NULL;

	scf_list_init(&bb->list);
	scf_list_init(&bb->dag_list_head);
	scf_list_init(&bb->code_list_head);
	scf_list_init(&bb->save_list_head);

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

	bb->dn_colors_entry = scf_vector_alloc();
	if (!bb->dn_colors_entry)
		goto error_colors_entry;

	bb->dn_colors_exit = scf_vector_alloc();
	if (!bb->dn_colors_exit)
		goto error_colors_exit;

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

	bb->ds_malloced = scf_vector_alloc();
	if (!bb->ds_malloced)
		goto error_malloced;

	bb->ds_freed = scf_vector_alloc();
	if (!bb->ds_freed)
		goto error_freed;

	bb->generate_flag = 1;
	return bb;

error_freed:
	scf_vector_free(bb->ds_malloced);
error_malloced:
	scf_vector_free(bb->dn_resaves);
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
	scf_vector_free(bb->dn_colors_exit);
error_colors_exit:
	scf_vector_free(bb->dn_colors_entry);
error_colors_entry:
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
		scf_list_clear(&bb->save_list_head, scf_3ac_code_t, list, scf_3ac_code_free);

		// DAG nodes were freed by 'scf_list_clear' above, so only free this vector
		if (bb->var_dag_nodes)
			scf_vector_free(bb->var_dag_nodes);

		if (bb->prevs)
			scf_vector_free(bb->prevs);

		if (bb->nexts)
			scf_vector_free(bb->nexts);
	}
}

scf_bb_group_t* scf_bb_group_alloc()
{
	scf_bb_group_t* bbg = calloc(1, sizeof(scf_bb_group_t));
	if (!bbg)
		return NULL;

	bbg->body = scf_vector_alloc();
	if (!bbg->body) {
		free(bbg);
		return NULL;
	}

	return bbg;
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
		SCF_BB_PRINT(save_list_head);
	}
}

void scf_bb_group_print(scf_bb_group_t* bbg)
{
	scf_basic_block_t* bb;
	int i;

	printf("\033[33mbbg: %p\033[0m\n", bbg);

	printf("\033[34mentries:\033[0m\n");
	if (bbg->entries) {
		for (i = 0; i < bbg->entries->size; i++) {
			bb =        bbg->entries->data[i];

			printf("%p, %d\n", bb, bb->index);
		}
	}

	printf("\033[35mbody:\033[0m\n");
	for (i = 0; i < bbg->body->size; i++) {
		bb =        bbg->body->data[i];

		printf("%p, %d\n", bb, bb->index);
	}

	printf("\033[36mexits:\033[0m\n");
	if (bbg->exits) {
		for (i = 0; i < bbg->exits->size; i++) {
			bb =        bbg->exits->data[i];

			printf("%p, %d\n", bb, bb->index);
		}
	}
	printf("\n");
}

void scf_basic_block_print_list(scf_list_t* h)
{
	scf_list_t* l;

	if (!scf_list_empty(h)) {

		scf_basic_block_t* bb2;
		scf_basic_block_t* last_bb  = scf_list_data(scf_list_tail(h), scf_basic_block_t, list);
		scf_list_t*        sentinel = scf_list_sentinel(&last_bb->code_list_head);

		for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

			scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

			printf("\033[33mbasic_block: %p, index: %d, dfo_normal: %d, cmp_flag: %d, group: %d, loop: %d\033[0m\n",
					bb, bb->index, bb->dfo_normal, bb->cmp_flag, bb->group_flag, bb->loop_flag);

			scf_basic_block_print(bb, sentinel);

			printf("\n");
			int i;
			if (bb->prevs) {
				for (i = 0; i < bb->prevs->size; i++) {
					bb2       = bb->prevs->data[i];

					printf("prev     : %p, index: %d\n", bb2, bb2->index);
				}
			}
			if (bb->nexts) {
				for (i = 0; i < bb->nexts->size; i++) {
					bb2       = bb->nexts->data[i];

					printf("next     : %p, index: %d\n", bb2, bb2->index);
				}
			}
#if 0
			if (bb->dominators) {
				for (i = 0; i < bb->dominators->size; i++)
					printf("dominator: %p\n", bb->dominators->data[i]);
			}
			if (bb->dn_status_initeds) {
				for (i = 0; i < bb->dn_status_initeds->size; i++) {

					scf_dn_status_t* av = bb->dn_status_initeds->data[i];
					scf_dag_node_t*   dn = av->dag_node;
					scf_variable_t*   v  = dn->var;

					if (v && v->w)
						printf("inited: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}
#endif
#if 1
			if (bb->ds_malloced) {
				scf_dn_status_t* ds;

				for (i = 0; i < bb->ds_malloced->size; i++) {
					ds =        bb->ds_malloced->data[i];

					if (scf_vector_find_cmp(bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
						continue;
					scf_dn_status_print(ds);
				}
			}
#endif
			if (bb->entry_dn_actives) {
				for (i = 0; i < bb->entry_dn_actives->size; i++) {

					scf_dag_node_t* dn = bb->entry_dn_actives->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("entry active: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->exit_dn_actives) {
				for (i = 0; i < bb->exit_dn_actives->size; i++) {

					scf_dag_node_t* dn = bb->exit_dn_actives->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("exit  active: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->entry_dn_aliases) {
				for (i = 0; i < bb->entry_dn_aliases->size; i++) {

					scf_dag_node_t* dn = bb->entry_dn_aliases->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("entry alias:  v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}
			if (bb->exit_dn_aliases) {
				for (i = 0; i < bb->exit_dn_aliases->size; i++) {

					scf_dag_node_t* dn = bb->exit_dn_aliases->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("exit  alias:  v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->dn_updateds) {
				for (i = 0; i < bb->dn_updateds->size; i++) {

					scf_dag_node_t* dn = bb->dn_updateds->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("updated:      v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->dn_loads) {
				for (i = 0; i < bb->dn_loads->size; i++) {

					scf_dag_node_t* dn = bb->dn_loads->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("loads:        v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->dn_reloads) {
				for (i = 0; i < bb->dn_reloads->size; i++) {

					scf_dag_node_t* dn = bb->dn_reloads->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("reloads:      v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->dn_saves) {
				for (i = 0; i < bb->dn_saves->size; i++) {

					scf_dag_node_t* dn = bb->dn_saves->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("saves:        v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			if (bb->dn_resaves) {
				for (i = 0; i < bb->dn_resaves->size; i++) {

					scf_dag_node_t* dn = bb->dn_resaves->data[i];
					scf_variable_t* v  = dn->var;

					if (v && v->w)
						printf("resaves:      v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
				}
			}

			printf("\n");
		}
	}
}

static int _copy_to_active_vars(scf_vector_t* active_vars, scf_vector_t* dag_nodes)
{
	scf_dag_node_t*   dn;
	scf_dn_status_t* status;

	int i;

	for (i = 0; i < dag_nodes->size; i++) {

		dn     = dag_nodes->data[i];

		status = scf_vector_find_cmp(active_vars, dn, scf_dn_status_cmp);

		if (!status) {
			status = scf_dn_status_alloc(dn);
			if (!status)
				return -ENOMEM;

			int ret = scf_vector_add(active_vars, status);
			if (ret < 0) {
				scf_dn_status_free(status);
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
		scf_dn_status_t* v  = active_vars->data[j];
		scf_dag_node_t*   dn = v->dag_node;

		if (scf_variable_const(dn->var))
			continue;

		if (!v->active)
			continue;

		if (scf_dn_through_bb(dn)) {
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
		scf_dn_status_t* v  = active_vars->data[j];
		scf_dag_node_t*   dn = v->dag_node;

		if (scf_variable_const(dn->var))
			continue;

		if (!v->updated)
			continue;

		if (scf_dn_through_bb(dn)) {
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

		if (c->dsts) {
			scf_3ac_operand_t* dst;
			int j;

			for (j  = 0; j < c->dsts->size; j++) {
				dst =        c->dsts->data[j];

				if (!dst->dag_node)
					continue;
			
				dn  = dst->dag_node;

				if (scf_dn_through_bb(dn)) {
					ret = scf_vector_add_unique(bb->var_dag_nodes, dn);
					if (ret < 0)
						return ret;
				}
			}
		}

		if (c->srcs) {
			scf_3ac_operand_t* src;
			scf_variable_t*    v;
			int j;

			for (j  = 0; j < c->srcs->size; j++) {
				src =        c->srcs->data[j];

				if (!src->dag_node)
					continue;

				dn = src->dag_node;
				v  = dn->var;

				if (scf_type_is_operator(dn->type)
						|| !scf_variable_const(v)
						|| (v->nb_dimentions > 0 && (SCF_OP_ARRAY_INDEX == c->op->type || scf_type_is_assign_array_index(c->op->type)))
						) {

					ret = scf_vector_add_unique(bb->var_dag_nodes, dn);
					if (ret < 0)
						return ret;
				}
			}
		}
	}

	return 0;
}

int scf_basic_block_dag2(scf_basic_block_t* bb, scf_list_t* dag_list_head)
{
	scf_list_t* l;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		int ret = scf_3ac_code_to_dag(c, dag_list_head);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int scf_basic_block_dag(scf_basic_block_t* bb, scf_list_t* bb_list_head, scf_list_t* dag_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	int ret;

	ret = scf_basic_block_dag2(bb, dag_list_head);
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
			scf_vector_clear(c->active_vars, (void (*)(void*))scf_dn_status_free);
		else {
			c->active_vars = scf_vector_alloc();
			if (!c->active_vars)
				return -ENOMEM;
		}

		ret = _copy_to_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			return ret;
	}

	return scf_basic_block_inited_vars(bb, bb_list_head);
}

static int _bb_init_pointer_aliases(scf_dn_status_t* ds_pointer, scf_dag_node_t* dn_alias, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;
	scf_dn_status_t*  ds3;
	scf_vector_t*      aliases;

	int ret;
	int i;

	scf_dn_status_vector_clear_by_ds(c ->dn_status_initeds, ds_pointer);
	scf_dn_status_vector_clear_by_ds(bb->dn_status_initeds, ds_pointer);

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = scf_pointer_alias(aliases, dn_alias, c, bb, bb_list_head);
	if (ret < 0) {
		scf_loge("\n");
		scf_vector_free(aliases);
		return ret;
	}

	ret = 0;

	for (i = 0; i < aliases->size; i++) {
		ds = aliases->data[i];

		ds2 = scf_dn_status_null();
		if (!ds2) {
			ret = -ENOMEM;
			break;
		}
		ds2->inited = 1;

		ret = scf_dn_status_copy_dn(ds2, ds_pointer);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			break;
		}

		ret = scf_dn_status_copy_alias(ds2, ds);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			break;
		}

		ret = scf_vector_add(c->dn_status_initeds, ds2);
		if (ret < 0) {
			scf_dn_status_free(ds2);
			break;
		}

		ds3 = scf_dn_status_ref(ds2);

		ret = scf_vector_add(bb->dn_status_initeds, ds3);
		if (ret < 0) {
			scf_dn_status_free(ds3);
			break;
		}
	}

	scf_vector_clear(aliases, NULL);
	scf_vector_free (aliases);
	return ret;
}

static int _bb_init_var(scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;
	scf_3ac_operand_t* src;

	if (!c->dn_status_initeds) {
		c ->dn_status_initeds = scf_vector_alloc();

		if (!c->dn_status_initeds)
			return -ENOMEM;
	}

	if (dn->var->nb_pointers > 0) {
		int ret;

		ds = scf_dn_status_alloc(dn);
		if (!ds)
			return -ENOMEM;

		assert(c->srcs && 1 == c->srcs->size);
		src = c->srcs->data[0];

		ret = _bb_init_pointer_aliases(ds, src->dag_node, c, bb, bb_list_head);
		scf_dn_status_free(ds);
		return ret;
	}

	SCF_DN_STATUS_GET(ds, c->dn_status_initeds, dn);
	ds->inited = 1;

	SCF_DN_STATUS_GET(ds2, bb->dn_status_initeds, dn);
	ds2->inited = 1;
	return 0;
}

static int _bb_init_array_index(scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_3ac_operand_t* base;
	scf_3ac_operand_t* index;
	scf_3ac_operand_t* src;

	scf_dag_node_t*    dn_base;
	scf_dag_node_t*    dn_index;
	scf_dag_node_t*    dn_src;

	scf_dn_status_t*  ds;
	scf_dn_status_t*  ds2;

	if (!c->dn_status_initeds) {
		c ->dn_status_initeds = scf_vector_alloc();

		if (!c->dn_status_initeds)
			return -ENOMEM;
	}

	ds = scf_dn_status_null(); 
	if (!ds)
		return -ENOMEM;

	ds->dn_indexes = scf_vector_alloc();
	if (!ds->dn_indexes) {
		scf_dn_status_free(ds);
		return -ENOMEM;
	}

	int type;
	switch (c->op->type) {
		case SCF_OP_3AC_ASSIGN_ARRAY_INDEX:
			assert(4 == c->srcs->size);
			src      =  c->srcs->data[3];
			dn_src   =  src->dag_node;
			type     =  SCF_OP_ARRAY_INDEX;
			break;

		case SCF_OP_3AC_ASSIGN_POINTER:
			assert(3 == c->srcs->size);
			src      =  c->srcs->data[2];
			dn_src   =  src->dag_node;
			type     =  SCF_OP_POINTER;
			break;
		default:
			scf_loge("\n");
			return -1;
			break;
	};

	base     = c->srcs->data[0];
	index    = c->srcs->data[1];

	dn_base  = base ->dag_node;
	dn_index = index->dag_node;

	int ret = scf_dn_status_index(ds, dn_index, type);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	while (SCF_OP_ARRAY_INDEX == dn_base->type
			|| SCF_OP_POINTER == dn_base->type) {

		dn_index = dn_base->childs->data[1];

		ret = scf_dn_status_index(ds, dn_index, dn_base->type);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		dn_base  = dn_base->childs->data[0];
	}

	assert(scf_type_is_var(dn_base->type));

	ds->dag_node = dn_base;
	ds->inited   = 1;

	if (scf_ds_is_pointer(ds) > 0)
		ret = _bb_init_pointer_aliases(ds, dn_src, c, bb, bb_list_head);
	else
		ret = 0;

	scf_dn_status_free(ds);
	return ret;
}

int scf_basic_block_inited_vars(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	int ret = 0;
	int i;

	scf_list_t*        l;
	scf_3ac_code_t*    c;
	scf_dag_node_t*    dn;
	scf_dn_status_t*   status;
	scf_dn_status_t*   status2;
	scf_3ac_operand_t* dst;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (!c->active_vars)
			continue;

		if (c->dsts && SCF_OP_ASSIGN == c->op->type) {

			dst = c->dsts->data[0];
			dn  = dst->dag_node;

			if ((scf_type_is_var(dn->type)
						|| SCF_OP_INC == dn->type || SCF_OP_INC_POST == dn->type
						|| SCF_OP_DEC == dn->type || SCF_OP_DEC_POST == dn->type)
					&& (dn->var->global_flag || dn->var->local_flag || dn->var->tmp_flag)) {

				scf_variable_t* v = dn->var;
				scf_logd("init: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

				ret = _bb_init_var(dn, c, bb, bb_list_head);
				if (ret < 0) {
					scf_loge("\n");
					return ret;
				}
			}
		} else if (SCF_OP_3AC_ASSIGN_ARRAY_INDEX == c->op->type
				|| SCF_OP_3AC_ASSIGN_POINTER     == c->op->type) {

			ret = _bb_init_array_index(c, bb, bb_list_head);
			if (ret < 0) {
				scf_loge("\n");
				scf_3ac_code_print(c, NULL);
				return ret;
			}
		}
	}

	return 0;
}

int scf_basic_block_active_vars(scf_basic_block_t* bb)
{
	scf_list_t*       l;
	scf_3ac_code_t*   c;
	scf_dag_node_t*   dn;

	int i;
	int j;
	int ret;

	ret = _bb_vars(bb);
	if (ret < 0)
		return ret;

	for (i = 0; i < bb->var_dag_nodes->size; i++) {
		dn =        bb->var_dag_nodes->data[i];

		if (scf_dn_through_bb(dn))
			dn->active  = 1;
		else
			dn->active  = 0;

		dn->updated = 0;
	}

	for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_prev(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (scf_type_is_jmp(c->op->type)
				|| SCF_OP_3AC_END == c->op->type)
			continue;

		if (c->dsts) {
			scf_3ac_operand_t* dst;

			for (j  = 0; j < c->dsts->size; j++) {
				dst =        c->dsts->data[j];

				if (scf_type_is_binary_assign(c->op->type))
					dst->dag_node->active = 1;
				else
					dst->dag_node->active = 0;
#if 1
				if (SCF_OP_3AC_LOAD != c->op->type
						&& SCF_OP_3AC_RELOAD != c->op->type)
#endif
					dst->dag_node->updated = 1;
			}
		}

		if (c->srcs) {
			scf_3ac_operand_t* src;

			for (j  = 0; j < c->srcs->size; j++) {
				src =        c->srcs->data[j];

				assert(src->dag_node);

				if (SCF_OP_ADDRESS_OF == c->op->type
						&& scf_type_is_var(src->dag_node->type))
					src->dag_node->active = 0;
				else
					src->dag_node->active = 1;

				if (SCF_OP_INC == c->op->type
						|| SCF_OP_INC_POST == c->op->type
						|| SCF_OP_3AC_INC  == c->op->type
						|| SCF_OP_DEC      == c->op->type
						|| SCF_OP_DEC_POST == c->op->type
						|| SCF_OP_3AC_DEC  == c->op->type)
					src->dag_node->updated = 1;
			}
		}

		if (c->active_vars)
			scf_vector_clear(c->active_vars, (void (*)(void*))scf_dn_status_free);
		else {
			c->active_vars = scf_vector_alloc();
			if (!c->active_vars)
				return -ENOMEM;
		}

		ret = _copy_to_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			return ret;
	}

#if 0
	scf_loge("bb: %d\n", bb->index);
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		if (scf_type_is_jmp(c->op->type))
			continue;
		scf_loge("\n");
		scf_3ac_code_print(c, NULL);

		for (j = 0; j < c->active_vars->size; j++) {

			scf_dn_status_t* ds = c->active_vars->data[j];
			scf_dag_node_t*  dn = ds->dag_node;
			scf_variable_t*  v  = dn->var;

			if (v->w)
				scf_logw("v_%d_%d/%s: active: %d\n", v->w->line, v->w->pos, v->w->text->data, ds->active);
			else
				scf_logw("v_%#lx, active: %d\n", 0xffff & (uintptr_t)v, ds->active);
		}
		scf_loge("\n\n");
	}
#endif

	scf_vector_clear(bb->entry_dn_actives, NULL);
	scf_vector_clear(bb->dn_updateds,      NULL);

	if (!scf_list_empty(&bb->code_list_head)) {

		l = scf_list_head(&bb->code_list_head);
		c = scf_list_data(l, scf_3ac_code_t, list);

		ret = _copy_from_active_vars(bb->entry_dn_actives, c->active_vars);
		if (ret < 0)
			return ret;

		ret = _copy_updated_vars(bb->dn_updateds, c->active_vars);
		if (ret < 0)
			return ret;

		for (i = 0; i < bb->dn_updateds->size; i++) {
			dn =        bb->dn_updateds->data[i];

			if (!dn->var->global_flag)
				continue;

			ret = scf_vector_add_unique(bb->exit_dn_actives, dn);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

int scf_basic_block_split(scf_basic_block_t* bb_parent, scf_basic_block_t** pbb_child)
{
	scf_list_t*        l;
	scf_basic_block_t* bb_child;
	scf_basic_block_t* bb_next;

	int ret;
	int i;
	int j;

	bb_child = scf_basic_block_alloc();
	if (!bb_child)
		return -ENOMEM;

	SCF_XCHG(bb_child->nexts, bb_parent->nexts);

	ret = scf_vector_add(bb_parent->nexts, bb_child);
	if (ret < 0) {
		scf_basic_block_free(bb_child);
		return ret;
	}

	ret = scf_vector_add(bb_child->prevs, bb_parent);
	if (ret < 0) {
		scf_basic_block_free(bb_child);
		return ret;
	}

	if (bb_parent->var_dag_nodes) {
		bb_child->var_dag_nodes = scf_vector_clone(bb_parent->var_dag_nodes);

		if (!bb_child->var_dag_nodes) {
			scf_basic_block_free(bb_child);
			return -ENOMEM;
		}
	}

	for (i = 0; i < bb_child->nexts->size; i++) {
		bb_next =   bb_child->nexts->data[i];

		for (j = 0; j < bb_next->prevs->size; j++) {

			if (bb_next->prevs->data[j] == bb_parent)
				bb_next->prevs->data[j] =  bb_child;
		}
	}

	*pbb_child = bb_child;
	return 0;
}

void scf_basic_block_mov_code(scf_list_t* start, scf_basic_block_t* bb_dst, scf_basic_block_t* bb_src)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	for (l = start; l != scf_list_sentinel(&bb_src->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb_dst->code_list_head, &c->list);

		c->basic_block = bb_dst;
	}
}

int scf_basic_block_search_bfs(scf_basic_block_t* root, scf_basic_block_bfs_pt find, void* data)
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

		ret = find(bb, data, queue);
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

int scf_basic_block_search_dfs_prev(scf_basic_block_t* root, scf_basic_block_dfs_pt find, void* data, scf_vector_t* results)
{
	scf_basic_block_t* bb;

	int i;
	int j;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	for (i = 0; i < root->prevs->size; ++i) {

		bb = root->prevs->data[i];

		if (bb->visited_flag)
			continue;

		ret = find(bb, data);
		if (ret < 0)
			return ret;

		if (ret > 0) {
			ret = scf_vector_add(results, bb);
			if (ret < 0)
				return ret;

			bb->visited_flag = 1;
			continue;
		}

		ret = scf_basic_block_search_dfs_prev(bb, find, data, results);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

int scf_basic_block_loads_saves(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dag_node_t* dn;
	scf_list_t*     l = &bb->list;

	int ret;
	int i;

	for (i = 0; i < bb->entry_dn_actives->size; i++) {

		dn = bb->entry_dn_actives->data[i];

		if (dn->var->extra_flag)
			continue;

		if (scf_vector_find(bb->entry_dn_aliases, dn)
				|| dn->var->tmp_flag)
			ret = scf_vector_add_unique(bb->dn_reloads, dn);
		else
			ret = scf_vector_add_unique(bb->dn_loads, dn);
		if (ret < 0)
			return ret;
	}

	for (i = 0; i < bb->exit_dn_actives->size; i++) {

		dn = bb->exit_dn_actives->data[i];

		if (!scf_vector_find(bb->dn_updateds, dn)) {
			if (l != scf_list_head(bb_list_head) || !dn->var->arg_flag)
				continue;
		}

		if (scf_vector_find(bb->exit_dn_aliases, dn)
				|| dn->var->tmp_flag)
			ret = scf_vector_add_unique(bb->dn_resaves, dn);
		else
			ret = scf_vector_add_unique(bb->dn_saves, dn);

		if (ret < 0)
			return ret;
	}

	for (i = 0; i < bb->exit_dn_aliases->size; i++) {

		dn = bb->exit_dn_aliases->data[i];

		if (!scf_vector_find(bb->dn_updateds, dn)) {
			if (l != scf_list_head(bb_list_head) || !dn->var->arg_flag)
				continue;
		}

		ret = scf_vector_add_unique(bb->dn_resaves, dn);
		if (ret < 0)
			return ret;
	}

	return 0;
}

