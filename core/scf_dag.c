#include"scf_dag.h"
#include"scf_3ac.h"

scf_active_var_t* scf_active_var_alloc(scf_dag_node_t* dn)
{
	if (!dn)
		return NULL;

	scf_active_var_t* v = calloc(1, sizeof(scf_active_var_t));
	if (!v)
		return NULL;

	v->dag_node   = dn;

	v->alias      = dn->alias;
	v->index      = dn->index;
	v->member     = dn->member;
	v->alias_type = dn->alias_type;

	v->active     = dn->active;
	v->inited     = dn->inited;
	v->updated    = dn->updated;

	return v;
}

void scf_active_var_free(scf_active_var_t* v)
{
	if (v) {
		free(v);
		v = NULL;
	}
}

scf_dag_node_t* scf_dag_node_alloc(int type, scf_variable_t* var)
{
	scf_dag_node_t* dag_node = calloc(1, sizeof(scf_dag_node_t));
	if (!dag_node)
		return NULL;

	dag_node->type = type;
	if (var)
		dag_node->var = scf_variable_clone(var);
	else
		dag_node->var = NULL;

#if 0
	if (1 || scf_type_is_var(type)) {
		scf_logw("dag_node: %#lx, dag_node->type: %d", 0xffff & (uintptr_t)dag_node, dag_node->type);
		if (var) {
			printf(", var->type: %d", var->type);
			if (var->w)
				printf(", v_%d_%d/%s", var->w->line, var->w->pos, var->w->text->data);
			else {
				//printf(", v_%#lx", 0xffff & (uintptr_t)var);
			}
		}
		printf("\n");
	}
#endif
	return dag_node;
}

int scf_dag_node_add_child(scf_dag_node_t* parent, scf_dag_node_t* child)
{
	if (!parent || !child)
		return -EINVAL;

	if (!parent->childs) {
		parent->childs = scf_vector_alloc();
		if (!parent->childs)
			return -ENOMEM;
	}

	if (!child->parents) {
		child->parents = scf_vector_alloc();
		if (!child->parents)
			return -ENOMEM;
	}

	int ret = scf_vector_add(parent->childs, child);
	if (ret < 0)
		return ret;

	ret = scf_vector_add(child->parents, parent);
	if (ret < 0) {
		scf_vector_del(parent->childs, child);
		return ret;
	}

	return 0;
}

void scf_dag_node_free(scf_dag_node_t* dag_node)
{
	if (dag_node) {
		if (dag_node->var)
			scf_variable_free(dag_node->var);

		if (dag_node->parents)
			scf_vector_free(dag_node->parents);

		if (dag_node->childs)
			scf_vector_free(dag_node->childs);

		free(dag_node);
		dag_node = NULL;
	}
}

void scf_dag_node_free_list(scf_list_t* dag_list_head)
{
	scf_list_t* l;

	for (l = scf_list_head(dag_list_head); l != scf_list_sentinel(dag_list_head); ) {

		scf_dag_node_t* dn = scf_list_data(l, scf_dag_node_t, list);

		l = scf_list_next(l);

		scf_list_del(&dn->list);

		scf_dag_node_free(dn);
		dn = NULL;
	}
}

int scf_dag_node_same(scf_dag_node_t* dag_node, const scf_node_t* node)
{
	int i;

	if (dag_node->type != node->type)
		return 0;

	if (SCF_OP_ADDRESS_OF == node->type) {
		scf_logd("type: %d, %d, node: %p, %p\n", dag_node->type, node->type, dag_node, node);
		scf_logd("var: %p, %p\n", dag_node->var, node->var);
//		scf_logi("var: %#lx, %#lx\n", 0xffff & (uintptr_t)dag_node->var, 0xffff & (uintptr_t)node->var);
	}

	if (scf_type_is_var(node->type)) {
		if (dag_node->var == node->var)
			return 1;
		else
			return 0;
	}

	if (!dag_node->childs)
		return 0;

	if (SCF_OP_TYPE_CAST == node->type) {
		scf_dag_node_t* dn0 = dag_node->childs->data[0];
		scf_variable_t* vn1 = _scf_operand_get(node->nodes[1]);

		if (dn0->var == vn1
				&& scf_variable_same_type(dag_node->var, node->result))
			return 1;
		else {
			scf_loge("var: %#lx, %#lx, type: %d, %d, node: %#lx, %#lx, same: %d\n",
					0xffff & (uintptr_t)dn0->var,
					0xffff & (uintptr_t)vn1,
					dag_node->var->type, node->result->type,
					0xffff & (uintptr_t)dag_node,
					0xffff & (uintptr_t)node,
					scf_variable_same_type(dag_node->var, node->result)
					);
			return 0;
		}
	} else if (SCF_OP_ARRAY_INDEX == node->type) {
		assert(3 == dag_node->childs->size);
		assert(2 == node->nb_nodes);
		goto cmp_childs;

	} else if (SCF_OP_ADDRESS_OF == node->type) {
		assert(1 == node->nb_nodes);

		if (SCF_OP_ARRAY_INDEX == node->nodes[0]->type) {
			assert(3 == dag_node->childs->size);
			assert(2 == node->nodes[0]->nb_nodes);
			node = node->nodes[0];
			goto cmp_childs;

		} else if (SCF_OP_POINTER == node->nodes[0]->type) {
			assert(2 == node->nodes[0]->nb_nodes);

			if (!dag_node->childs || 2 != dag_node->childs->size)
				return 0;

			node = node->nodes[0];
			goto cmp_childs;
		}
	}

	if (dag_node->childs->size != node->nb_nodes) {
		if (SCF_OP_ADDRESS_OF == node->type) {
			scf_loge("node: %p, %p, size: %d, %d\n", dag_node, node, dag_node->childs->size, node->nb_nodes);
		}
		return 0;
	}

cmp_childs:
	for (i = 0; i < node->nb_nodes; i++) {
		scf_dag_node_t*	dag_child = dag_node->childs->data[i];
		scf_node_t*		child = node->nodes[i];

		while (SCF_OP_EXPR == child->type)
			child = child->nodes[0];

		if (0 == scf_dag_node_same(dag_child, child))
			return 0;
	}

	return 1;
}

scf_dag_node_t* scf_dag_find_node(scf_list_t* h, const scf_node_t* node)
{
	scf_node_t* origin = (scf_node_t*)node;

	while (SCF_OP_EXPR == origin->type)
		origin = origin->nodes[0];

	scf_list_t* l;
	for (l = scf_list_tail(h); l != scf_list_sentinel(h); l = scf_list_prev(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (scf_dag_node_same(dag_node, origin))
			return dag_node;
	}

	return NULL;
}

int scf_dag_get_node(scf_list_t* h, const scf_node_t* node, scf_dag_node_t** pp)
{
	scf_dag_node_t* dag_node = scf_dag_find_node(h, node);

	if (!dag_node) {
		dag_node = scf_dag_node_alloc(node->type, _scf_operand_get((scf_node_t*)node));
		if (!dag_node)
			return -ENOMEM;

		scf_list_add_tail(h, &dag_node->list);
	}

	*pp = dag_node;
	return 0;
}

int scf_dag_dn_same(scf_dag_node_t* dn0, scf_dag_node_t* dn1)
{
	int i;

	if (dn0->type != dn1->type)
		return 0;

	if (scf_type_is_var(dn0->type)) {
		if (dn0->var == dn1->var)
			return 1;
		else
			return 0;
	}

	if (!dn0->childs) {
		if (dn1->childs)
			return 0;

		scf_logd("dn0: %p, %p, dn1: %p, %p, var: %p, %p\n", dn0, dn0->childs, dn1, dn1->childs, dn0->var, dn1->var);
		if (dn0->var == dn1->var)
			return 1;
		return 0;
	} else if (!dn1->childs)
		return 0;

	if (dn0->childs->size != dn1->childs->size) {
		scf_logd("dn0: %p, %d, dn1: %p, %d\n", dn0, dn0->childs->size, dn1, dn1->childs->size);
		return 0;
	}

	for (i = 0; i < dn0->childs->size; i++) {
		scf_dag_node_t*	child0 = dn0->childs->data[i];
		scf_dag_node_t*	child1 = dn1->childs->data[i];

		if (0 == scf_dag_dn_same(child0, child1))
			return 0;
	}
	return 1;
}

scf_dag_node_t* scf_dag_find_dn(scf_list_t* h, const scf_dag_node_t* dn0)
{
	scf_list_t* l;
	for (l = scf_list_tail(h); l != scf_list_sentinel(h); l = scf_list_prev(l)) {

		scf_dag_node_t* dn1 = scf_list_data(l, scf_dag_node_t, list);

		if (SCF_OP_ADDRESS_OF == dn0->type && SCF_OP_ADDRESS_OF == dn1->type) {
			scf_logd("origin dn0: %p, %s, dn1: %p, %s\n",
					dn0, dn0->var->w->text->data,
					dn1, dn1->var->w->text->data);
		}

		if (scf_dag_dn_same(dn1, (scf_dag_node_t*)dn0))
			return dn1;
	}
	return NULL;
}

int scf_dag_get_dn(scf_list_t* h, const scf_dag_node_t* dn0, scf_dag_node_t** pp)
{
	scf_dag_node_t* dn1 = scf_dag_find_dn(h, dn0);

	if (!dn1) {
		dn1 = scf_dag_node_alloc(dn0->type, dn0->var);
		if (!dn1)
			return -ENOMEM;

		scf_list_add_tail(h, &dn1->list);
	}

	*pp = dn1;
	return 0;
}

int scf_dag_find_roots(scf_list_t* h, scf_vector_t* roots)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->parents) {
			int ret = scf_vector_add(roots, dag_node);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_dag_find_leafs(scf_list_t* h, scf_vector_t* leafs)
{
	scf_list_t* l;
	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_dag_node_t* dag_node = scf_list_data(l, scf_dag_node_t, list);

		if (!dag_node->childs) {
			int ret = scf_vector_add_unique(leafs, dag_node);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_dag_root_find_leafs(scf_dag_node_t* root, scf_vector_t* leafs)
{
	if (!root->childs)
		return scf_vector_add_unique(leafs, root);

	int i;
	for (i = 0; i < root->childs->size; i++) {

		scf_dag_node_t* child = root->childs->data[i];

		int ret = scf_dag_root_find_leafs(child, leafs);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int scf_dag_vector_find_leafs(scf_vector_t* roots, scf_vector_t* leafs)
{
	int i;
	for (i = 0; i < roots->size; i++) {

		scf_dag_node_t* root = roots->data[i];

		int ret = scf_dag_root_find_leafs(root, leafs);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int scf_dag_pointer_alias(scf_active_var_t* v, scf_dag_node_t* dn, scf_3ac_code_t* c)
{
	assert(SCF_OP_ASSIGN == c->op->type
			|| SCF_OP_DEREFERENCE == c->op->type);

	assert(c->srcs && c->srcs->size > 0);

	scf_3ac_operand_t* src    = c->srcs->data[0];
	scf_dag_node_t*    dn_src = src->dag_node;
	scf_dag_node_t*    alias;

	while (SCF_OP_TYPE_CAST == dn_src->type) {
		assert(dn_src->childs && 1 == dn_src->childs->size);
		dn_src = dn_src->childs->data[0];
	}

	if (SCF_OP_ADDRESS_OF == dn_src->type) {

		alias = dn_src->childs->data[0];
		switch (dn_src->childs->size) {
			case 1:
				assert(scf_type_is_var(alias->type));
				v->alias      = alias;
				v->alias_type = SCF_DN_ALIAS_VAR;
				break;
			case 2:
				v->alias      = NULL;
				v->alias_type = SCF_DN_ALIAS_MEMBER;
				break;
			case 3:
				v->alias      = NULL;
				v->alias_type = SCF_DN_ALIAS_ARRAY;
				break;
			default:
				scf_loge("\n");
				return -1;
				break;
		};
	} else if (SCF_OP_ADD == dn_src->type) {
		scf_loge("\n");
		return -1;
	} else if (scf_type_is_var(dn_src->type)) {

		if (dn_src->var->nb_dimentions > 0) {
			v->alias      = dn_src;
			v->index      = 0;
			v->alias_type = SCF_DN_ALIAS_ARRAY;

		} else if (dn_src->var->nb_pointers > 0) {
			scf_loge("\n");
			return -1;
		} else {
			v->alias      = dn_src;
			v->alias_type = SCF_DN_ALIAS_VAR;
		}
	} else {
		if (dn_src->var->w)
			scf_logw("type: %d, v_%s\n", dn_src->type, dn_src->var->w->text->data);
		else
			scf_logw("type: %d, v_%#lx\n", dn_src->type, 0xffff & (uintptr_t)dn_src->var);
		return -1;
	}
	return 0;
}

