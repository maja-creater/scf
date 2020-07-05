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
	scf_list_init(&bb->load_list_head);
	scf_list_init(&bb->code_list_head);
	scf_list_init(&bb->save_list_head);

	bb->prevs = scf_vector_alloc();
	if (!bb->prevs) {
		free(bb);
		return NULL;
	}

	bb->nexts = scf_vector_alloc();
	if (!bb->nexts) {
		scf_vector_free(bb->prevs);
		free(bb);
		return NULL;
	}

	return bb;
}

void scf_basic_block_free(scf_basic_block_t* bb)
{
	if (bb) {
		// this bb's DAG nodes freed here
		scf_list_clear(&bb->dag_list_head, scf_dag_node_t, list, scf_dag_node_free);

		scf_list_clear(&bb->load_list_head, scf_3ac_code_t, list, scf_3ac_code_free);
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

void scf_basic_block_print(scf_basic_block_t* bb, scf_list_t* sentinel)
{
	if (bb) {
		scf_list_t* l;
		for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
				l = scf_list_next(l)) {

			scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

			scf_3ac_code_print(c, sentinel);
		}
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
		}
	}
	printf("end\n");
}

static int _copy_active_vars(scf_vector_t* active_vars, scf_vector_t* dag_nodes)
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

int scf_basic_block_active_vars(scf_basic_block_t* bb)
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
	scf_list_clear(&bb->load_list_head, scf_3ac_code_t, list, scf_3ac_code_free);
	scf_list_clear(&bb->save_list_head, scf_3ac_code_t, list, scf_3ac_code_free);

	ret = scf_dag_find_roots(&bb->dag_list_head, roots);
	if (ret < 0)
		goto error;

	ret = scf_dag_vector_find_leafs(roots, leafs);
	if (ret < 0)
		goto error;

	for (i = 0; i < leafs->size; i++) {
		scf_dag_node_t* dag_node = leafs->data[i];

		// it should be 'real' variable, not 'const' variable,
		// const pointer var can be changed, but what it points to can't be changed.
		if (!scf_variable_const(dag_node->var)) {
			dag_node->active = 1;

			ret = scf_vector_add(bb->var_dag_nodes, dag_node);
			if (ret < 0)
				goto error;

			// if node is var, but not function's argment var
			if (scf_type_is_var(dag_node->type) && !dag_node->var->arg_flag) {
				scf_logw("type: %d, var: %s, const_flag: %d, const_literal_flag: %d, nb_pointers: %d\n",
						dag_node->type,
						dag_node->var->w->text->data,
						dag_node->var->const_flag,
						dag_node->var->const_literal_flag,
						dag_node->var->nb_pointers
						);

				scf_3ac_code_t* load = scf_3ac_alloc_by_dst(SCF_OP_3AC_LOAD, dag_node);
				if (!load)
					goto error;
				scf_list_add_tail(&bb->load_list_head, &load->list);
				load->basic_block = bb;

				scf_3ac_code_t* save = scf_3ac_alloc_by_src(SCF_OP_3AC_SAVE, dag_node);
				if (!save)
					goto error;
				scf_list_add_tail(&bb->save_list_head, &save->list);
				save->basic_block = bb;
			}
		}
	}

	for (l = scf_list_tail(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_prev(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (scf_type_is_jmp(c->op->type))
			continue;

		if (c->dst && c->dst->dag_node)
			c->dst->dag_node->active = 0;

		if (c->srcs) {
			int j;
			for (j = 0; j < c->srcs->size; j++) {
				scf_3ac_operand_t* src = c->srcs->data[j];

				if (!src->node)
					continue;

				assert(src->dag_node);
				src->dag_node->active = 1;

				if (scf_type_is_operator(src->dag_node->type)
						|| !scf_variable_const(src->dag_node->var)) {

					scf_logw("type: %d, var: %s, const_flag: %d, const_literal_flag: %d, nb_pointers: %d\n",
							src->dag_node->type,
							src->dag_node->var->w->text->data,
							src->dag_node->var->const_flag,
							src->dag_node->var->const_literal_flag,
							src->dag_node->var->nb_pointers
							);

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

		ret = _copy_active_vars(c->active_vars, bb->var_dag_nodes);
		if (ret < 0)
			goto error;
	}
#if 0
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_print(c, NULL);

		for (j = 0; j < c->active_vars->size; j++) {
			scf_active_var_t* v = c->active_vars->data[j];
			if (v->dag_node->var) {
				if (v->dag_node->var->w)
					scf_logi("var: %s, active: %d\n", v->dag_node->var->w->text->data, v->active);
				else
					scf_logi("var: %p, active: %d\n", v->dag_node->var, v->active);
			}
		}
		printf("\n");
	}
#endif

	return 0;

error:
	scf_vector_free(leafs);
	scf_vector_free(roots);
	return ret;
}

static int _graph_node_cmp(const void* v0, const void* v1)
{
	const scf_dag_node_t*   dn = v0;
	const scf_graph_node_t* gn = v1;

	return dn != gn->data;
}

int scf_basic_block_kcolor_vars(scf_basic_block_t* bb, scf_vector_t* colors)
{
	if (!bb || !colors || 0 == colors->size)
		return -EINVAL;

	scf_graph_t* g = scf_graph_alloc();
	if (!g)
		return -ENOMEM;

	int ret = 0;
	int i;

	for (i = 0; i < bb->var_dag_nodes->size; i++) {

		scf_dag_node_t*   dn = bb->var_dag_nodes->data[i];

		scf_logd("dn: %p, type: %d\n", dn, dn->type);

		scf_graph_node_t* gn = scf_graph_node_alloc();
		if (!gn) {
			ret = -ENOMEM;
			goto error;
		}
		gn->data = dn;

		ret = scf_graph_add_node(g, gn);
		if (ret < 0) {
			scf_graph_node_free(gn);
			gn = NULL;
			goto error;
		}
	}

	scf_logi("\033[33mbasic_block: %p\033[0m\n", bb);

	scf_list_t* l;
	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_3ac_code_print(c, NULL);

		if (scf_type_is_jmp(c->op->type))
			continue;

		if (!c->dst || !c->dst->dag_node)
			continue;

		scf_logd("dn: %p, type: %d\n", c->dst->dag_node, c->dst->dag_node->type);

		scf_dag_node_t*   dn_dst = c->dst->dag_node;
		scf_graph_node_t* gn_dst = scf_vector_find_cmp(g->nodes, c->dst->dag_node, _graph_node_cmp);
		assert(gn_dst);

		for (i = 0; i < c->active_vars->size; i++) {

			scf_active_var_t*   active    = c->active_vars->data[i];
			scf_dag_node_t*     dn_active = active->dag_node;

			scf_graph_node_t*   gn_active = scf_vector_find_cmp(g->nodes, dn_active, _graph_node_cmp);
			assert(gn_active);

			if (gn_active == gn_dst)
				continue;

			if (!scf_vector_find(gn_dst->neighbors, gn_active)) {
				assert(!scf_vector_find(gn_active->neighbors, gn_dst));

				scf_logd("v_%d_%d/%s, v_%d_%d/%s\n",
						dn_active->var->w->line, dn_active->var->w->pos, dn_active->var->w->text->data,
						dn_dst->var->w->line, dn_dst->var->w->pos, dn_dst->var->w->text->data);

				ret = scf_graph_make_edge(gn_dst, gn_active);
				if (ret < 0)
					goto error;

				ret = scf_graph_make_edge(gn_active, gn_dst);
				if (ret < 0)
					goto error;
			} else
				assert(scf_vector_find(gn_active->neighbors, gn_dst));
		}
	}

	ret = scf_graph_kcolor(g, colors);
	if (ret < 0)
		goto error;

	for (i = 0; i < g->nodes->size; i++) {

		scf_graph_node_t* gn = g->nodes->data[i];
		scf_dag_node_t*   dn = gn->data;

		dn->color = gn->color;

		scf_logi("v_%d_%d/%s, color: %ld\n",
				dn->var->w->line, dn->var->w->pos, dn->var->w->text->data, dn->color);
	}
	printf("\n");

error:
	scf_graph_free(g);
	return ret;
}

