#include"scf_optimizer.h"

static int __bb_dfs_initeds(scf_basic_block_t* root, scf_dag_node_t* dn, scf_vector_t* initeds)
{
	scf_basic_block_t* bb;
	scf_active_var_t*  status;

	int i;
	int j;
	int ret;

	assert(!root->jmp_flag);

	root->visited_flag = 1;

	for (i = 0; i < root->prevs->size; ++i) {

		bb = root->prevs->data[i];

		if (bb->visited_flag)
			continue;

		for (j = 0; j < bb->dn_status_initeds->size; j++) {

			status = bb->dn_status_initeds->data[j];

			if (dn == status->dag_node)
				break;
		}

		if (j < bb->dn_status_initeds->size) {
			ret = scf_vector_add(initeds, bb);
			if (ret < 0)
				return ret;

			bb->visited_flag = 1;
			continue;
		}

		ret = __bb_dfs_initeds(bb, dn, initeds);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

static int __bb_dfs_check_initeds(scf_basic_block_t* root, scf_basic_block_t* obj)
{
	scf_basic_block_t* bb;

	int i;
	int ret;

	assert(!root->jmp_flag);

	if (root == obj)
		return -1;

	if (root->visited_flag)
		return 0;

	root->visited_flag = 1;

	for (i = 0; i < root->nexts->size; ++i) {

		bb = root->nexts->data[i];

		if (bb->visited_flag)
			continue;

		if (bb == obj)
			return -1;

		ret = __bb_dfs_check_initeds(bb, obj);
		if ( ret < 0)
			return ret;
	}

	return 0;
}

static int _bb_dfs_initeds(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dag_node_t* dn, scf_vector_t* initeds)
{
	scf_list_t*        l;
	scf_basic_block_t* bb2;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb2 = scf_list_data(l, scf_basic_block_t, list);

		bb2->visited_flag = 0;
	}

	return __bb_dfs_initeds(bb, dn, initeds);
}

static int _bb_dfs_check_initeds(scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_vector_t* initeds)
{
	scf_list_t*        l;
	scf_basic_block_t* bb2;

	int i;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head);
			l = scf_list_next(l)) {

		bb2 = scf_list_data(l, scf_basic_block_t, list);

		bb2->visited_flag = 0;
	}

	for (i = 0; i < initeds->size; i++) {
		bb2 = initeds->data[i];
		bb2->visited_flag = 1;
	}

	l = scf_list_head(bb_list_head);
	bb2 = scf_list_data(l, scf_basic_block_t, list);

	return __bb_dfs_check_initeds(bb2, bb);
}

static int _bb_pointer_initeds(scf_vector_t* initeds, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dag_node_t* dn)
{
	int ret;

	ret = _bb_dfs_initeds(bb_list_head, bb, dn, initeds);
	if (ret < 0)
		return ret;

	if (0 == initeds->size) {
		scf_loge("pointer v_%d_%d/%s is not inited\n",
				dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
		return -1;
	}

	ret = _bb_dfs_check_initeds(bb_list_head, bb, initeds);
	if (ret < 0) {
		scf_loge("in bb: %p, pointer v_%d_%d/%s may not be inited\n",
				bb, dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
	}
	return ret;
}

static int _bb_pointer_aliases(scf_vector_t* aliases, scf_list_t* bb_list_head, scf_basic_block_t* bb, scf_dag_node_t* dn_pointer)
{
	scf_vector_t*      initeds;
	scf_basic_block_t* bb2;
	scf_active_var_t*  status;

	int ret;
	int i;

	initeds = scf_vector_alloc();
	if (!initeds)
		return -ENOMEM;

	ret = _bb_pointer_initeds(initeds, bb_list_head, bb, dn_pointer);
	if (ret < 0) {
		scf_vector_free(initeds);
		return ret;
	}

	for (i = 0; i < initeds->size; i++) {

		bb2    = initeds->data[i];
		status = scf_vector_find_cmp(bb2->dn_status_initeds, dn_pointer, scf_dn_status_cmp);

		assert(status);

		if (SCF_DN_ALIAS_ARRAY == status->alias_type
				|| SCF_DN_ALIAS_MEMBER == status->alias_type)
			continue;
		assert(status->alias);

		if (scf_vector_find_cmp(aliases, status, scf_dn_status_cmp_alias))
			continue;

		ret = scf_vector_add(aliases, status);
		if (ret < 0) {
			scf_vector_free(initeds);
			return ret;
		}
	}

	scf_vector_free(initeds);
	return 0;
}

static int _3ac_pointer_alias(scf_dag_node_t* alias, scf_3ac_code_t* c, scf_basic_block_t* bb)
{
	scf_3ac_operand_t* pointer;

	int ret;

	assert(c->srcs && c->srcs->size >= 1);

	pointer = c->srcs->data[0];

	ret     = scf_vector_del(c->srcs, pointer);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

#if 1
	scf_3ac_code_t* c2;
	scf_3ac_code_t* c3;
	scf_list_t*     l2;
	scf_list_t*     l3;
	scf_list_t      h;

	scf_list_init(&h);

	ret = scf_dag_expr_calculate(&h, pointer->dag_node);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l2 = scf_list_head(&h); l2 != scf_list_sentinel(&h); ) {

		c2  = scf_list_data(l2, scf_3ac_code_t, list);
		l2  = scf_list_next(l2);

		for (l3 = scf_list_prev(&c->list); l3 != scf_list_sentinel(&bb->code_list_head); ) {

			c3  = scf_list_data(l3, scf_3ac_code_t, list);
			l3  = scf_list_prev(l3);

			if (scf_3ac_code_same(c2, c3)) {
				scf_list_del(&c3->list);
				scf_3ac_code_free(c3);
				break;
			}
		}

		scf_list_del(&c2->list);
		scf_3ac_code_free(c2);
	}
#endif

	pointer->dag_node = alias;
	SCF_XCHG(c->dst, pointer);
	scf_3ac_operand_free(pointer);
	pointer = NULL;

	switch (c->op->type) {
		case SCF_OP_3AC_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_ASSIGN);
			break;

		case SCF_OP_3AC_ADD_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_ADD_ASSIGN);
			break;
		case SCF_OP_3AC_SUB_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_SUB_ASSIGN);
			break;

		case SCF_OP_3AC_AND_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_AND_ASSIGN);
			break;
		case SCF_OP_3AC_OR_ASSIGN_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_OR_ASSIGN);
			break;

		case SCF_OP_3AC_INC_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_INC);
			break;
		case SCF_OP_3AC_DEC_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_DEC);
			break;
		case SCF_OP_3AC_INC_POST_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_INC_POST);
			break;
		case SCF_OP_3AC_DEC_POST_DEREFERENCE:
			c->op = scf_3ac_find_operator(SCF_OP_DEC_POST);
			break;

		default:
			scf_loge("\n");
			return -1;
			break;
	};
	assert(c->op);
	return 0;
}

static int _bb_split(scf_basic_block_t* bb_parent, scf_basic_block_t** pbb_child)
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

static int _bb_copy_aliases(scf_basic_block_t* bb, scf_dag_node_t* dn_pointer, scf_dag_node_t* dn_dereference, scf_vector_t* aliases)
{
	scf_dag_node_t*    dn_alias;
	scf_active_var_t*  status;
	scf_active_var_t*  status2;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		status   = aliases->data[i];
		dn_alias = status->alias;

//		scf_variable_t* v = dn_alias->var;
//		scf_logw("dn_alias: v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);

		if (SCF_DN_ALIAS_VAR == status->alias_type) {
			if (scf_type_is_var(dn_alias->type)
					&& !scf_variable_const(dn_alias->var)
					&& (dn_alias->var->global_flag || dn_alias->var->local_flag)) {

				ret = scf_vector_add_unique(bb->entry_dn_aliases, dn_alias);
				if (ret < 0)
					return ret;
			}
		}

		status2 = calloc(1, sizeof(scf_active_var_t));
		if (!status2)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_pointer_aliases, status2);
		if (ret < 0) {
			scf_active_var_free(status2);
			return ret;
		}

		status2->dag_node    = dn_pointer;
		status2->dereference = dn_dereference;

		status2->alias       = status->alias;
		status2->alias_type  = status->alias_type;
		status2->index       = status->index;
		status2->member      = status->member;
	}
	return 0;
}

static int __alias_dereference(scf_vector_t* aliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* status;
	scf_dag_node_t*   dn_alias;

	int count;
	int ret;
	int i;

	if (SCF_OP_DEREFERENCE == dn_pointer->type) {
		count = 0;

		for (i = 0; i < bb->dn_pointer_aliases->size; i++) {
			status    = bb->dn_pointer_aliases->data[i];

			if (dn_pointer != status->dereference)
				continue;

			dn_alias = status->alias;

			ret = __alias_dereference(aliases, dn_alias, c, bb, bb_list_head);
			if (ret < 0)
				return ret;

			++count;
		}
		if (count > 0)
			return 0;

		assert(dn_pointer->childs && 1 == dn_pointer->childs->size);

		scf_vector_t* aliases2 = scf_vector_alloc();
		if (!aliases2)
			return -ENOMEM;

		dn_alias = dn_pointer->childs->data[0];
		ret = __alias_dereference(aliases2, dn_alias, c, bb, bb_list_head);
		if (ret < 0) {
			scf_vector_free(aliases2);
			return ret;
		}

		for (i = 0; i < aliases2->size; i++) {
			dn_alias = aliases2->data[i];

			ret = __alias_dereference(aliases, dn_alias, c, bb, bb_list_head);
			if (ret < 0) {
				scf_vector_free(aliases2);
				return ret;
			}
		}

		scf_vector_free(aliases2);
		return 0;
	}

	if (!scf_type_is_var(dn_pointer->type)) {
		SCF_DN_STATUS_GET(status, c->active_vars, dn_pointer);

		ret = scf_dag_pointer_alias(status, dn_pointer, c);
		if (ret < 0)
			return ret;

		return scf_vector_add_unique(aliases, status);
	}

	status = scf_vector_find_cmp(c->active_vars, dn_pointer, scf_dn_status_cmp);
	if (status && status->alias)
		ret = scf_vector_add_unique(aliases, status);
	else
		ret = _bb_pointer_aliases(aliases, bb_list_head, bb, dn_pointer);
	return ret;
}

static int _alias_dereference(scf_vector_t** paliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_vector_t*      aliases;
	int ret;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __alias_dereference(aliases, dn_pointer, c, bb, bb_list_head);
	if (ret < 0) {
		scf_vector_free(aliases);
		return ret;
	}

	*paliases = aliases;
	return 0;
}

static int _alias_assign_dereference(scf_vector_t** paliases, scf_dag_node_t* dn_pointer, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_active_var_t* status;
	scf_vector_t*     aliases;
	int ret;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __alias_dereference(aliases, dn_pointer, c, bb, bb_list_head);
	if (ret < 0) {
		scf_vector_free(aliases);
		return ret;
	}

	if (1 == aliases->size) {
		status = aliases->data[0];

		if (SCF_DN_ALIAS_VAR == status->alias_type) {
			ret = _3ac_pointer_alias(status->alias, c, bb);
			scf_vector_free(aliases);
			if (ret < 0)
				return ret;
			return scf_basic_block_inited_vars(bb);
		}
		return 0;
	}

	*paliases = aliases;
	return 0;
}

static int __optimize_alias_bb(scf_list_t** pend, scf_list_t* start, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*        l;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* pointer;
	scf_dag_node_t*    dn_pointer;
	scf_dag_node_t*    dn_dereference;
	scf_vector_t*      aliases;

	int ret = 0;

	for (l = start; l != *pend; l = scf_list_next(l)) {

		c  = scf_list_data(l, scf_3ac_code_t, list);

		if (!scf_type_is_assign_dereference(c->op->type)
				&& SCF_OP_DEREFERENCE != c->op->type)
			continue;

		assert(c->srcs && c->srcs->size >= 1);
		pointer        = c->srcs->data[0];
		dn_pointer     = pointer->dag_node;
		aliases        = NULL;
		dn_dereference = NULL;

		if (SCF_OP_DEREFERENCE == c->op->type) {

			assert(c->dst && c->dst->dag_node);
			dn_dereference = c->dst->dag_node;

			ret = _alias_dereference(&aliases, dn_pointer, c, bb, bb_list_head);
		} else
			ret = _alias_assign_dereference(&aliases, dn_pointer, c, bb, bb_list_head);

		if (ret < 0)
			return ret;

		if (aliases) {
			ret = _bb_copy_aliases(bb, dn_pointer, dn_dereference, aliases);
			scf_vector_free(aliases);
			aliases = NULL;
			if (ret < 0)
				return ret;

			*pend = l;
			break;
		}
	}

	return 0;
}

static int _optimize_alias_bb(scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t* l;
	scf_list_t* start;
	scf_list_t* end;

	int ret;

	start = scf_list_head(&bb->code_list_head);
	end   = scf_list_sentinel(&bb->code_list_head);

	while (1) {
		ret     = __optimize_alias_bb(&end, start, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (end == scf_list_sentinel(&bb->code_list_head))
			break;

		scf_basic_block_t* bb_child = NULL;
		scf_3ac_code_t*    c;

		start = end;

		if (start != scf_list_head(&bb->code_list_head)) {

			for (l = scf_list_prev(start); l != scf_list_sentinel(&bb->code_list_head);
					l = scf_list_prev(l)) {

				c = scf_list_data(l, scf_3ac_code_t, list);

				if (scf_type_is_assign(c->op->type)
						|| scf_type_is_assign_dereference(c->op->type))
					break;
			}
			l = scf_list_next(l);

			if (l != scf_list_head(&bb->code_list_head)) {
				int ret = _bb_split(bb, &bb_child);
				if (ret < 0)
					return ret;

				bb_child->dereference_flag = 1;
				scf_list_add_front(&bb->list, &bb_child->list);

				SCF_XCHG(bb->entry_dn_aliases,   bb_child->entry_dn_aliases);
				SCF_XCHG(bb->dn_pointer_aliases, bb_child->dn_pointer_aliases);


				for ( ; l != scf_list_sentinel(&bb->code_list_head); ) {
					c = scf_list_data(l, scf_3ac_code_t, list);
					l = scf_list_next(l);

					scf_list_del(&c->list);
					scf_list_add_tail(&bb_child->code_list_head, &c->list);
				}

				bb    = bb_child;
			}
		}
		start = scf_list_next(start);

		for (l = start; l != scf_list_sentinel(&bb->code_list_head); ) {
			c = scf_list_data(l, scf_3ac_code_t, list);
			l = scf_list_next(l);

			if (scf_type_is_assign(c->op->type)
					|| scf_type_is_assign_dereference(c->op->type))
				break;
		}
		end = l;
	}
	return 0;
}

static int _optimize_pointer_alias(scf_function_t* f, scf_list_t* bb_list_head)
{
	if (!f || !bb_list_head)
		return -EINVAL;

	if (scf_list_empty(bb_list_head))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;

	int ret = 0;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		if (bb->jmp_flag || bb->end_flag || bb->cmp_flag)
			continue;

		if (!bb->dereference_flag)
			continue;

		ret = _optimize_alias_bb(bb, bb_list_head);
		if (ret < 0)
			return ret;
	}

//	scf_basic_block_print_list(bb_list_head);
	ret = 0;
error:
	return ret;
}

scf_optimizer_t  scf_optimizer_pointer_alias =
{
	.name     =  "pointer_alias",

	.optimize =  _optimize_pointer_alias,
};

