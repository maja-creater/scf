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
		assert(status && status->alias);

		ret = scf_vector_add_unique(aliases, status->alias);
		if (ret < 0) {
			scf_vector_free(initeds);
			return ret;
		}
	}

	scf_vector_free(initeds);
	return 0;
}

static int _3ac_pointer_alias(scf_dag_node_t* alias, scf_3ac_code_t* c)
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

static int _bb_copy_aliases(scf_basic_block_t* bb, scf_dag_node_t* dn_pointer, scf_vector_t* aliases)
{
	scf_dag_node_t*    dn_alias;
	scf_active_var_t*  status;

	int ret;
	int i;

	for (i = 0; i < aliases->size; i++) {
		dn_alias  = aliases->data[i];

		ret = scf_vector_add(bb->entry_dn_aliases, dn_alias);
		if (ret < 0)
			return ret;

		status = calloc(1, sizeof(scf_active_var_t));
		if (!status)
			return -ENOMEM;

		ret = scf_vector_add(bb->dn_pointer_aliases, status);
		if (ret < 0) {
			scf_active_var_free(status);
			return ret;
		}

		status->dag_node = dn_pointer;
		status->alias    = dn_alias;
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
	scf_list_t*        l2;
	scf_basic_block_t* bb;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* pointer;
	scf_dag_node_t*    dn_pointer;
	scf_vector_t*      aliases;
	scf_active_var_t*  status;

	int ret = 0;
	int i;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		if (bb->jmp_flag || bb->end_flag || bb->cmp_flag)
			continue;

		if (!bb->dereference_flag)
			continue;

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head);
				l2 = scf_list_next(l2)) {

			c  = scf_list_data(l2, scf_3ac_code_t, list);


			if (!scf_type_is_assign_dereference(c->op->type)
					&& SCF_OP_DEREFERENCE != c->op->type)
				continue;

			assert(c->srcs && c->srcs->size >= 1);
			pointer    = c->srcs->data[0];
			dn_pointer = pointer->dag_node;

			status = scf_vector_find_cmp(c->active_vars, dn_pointer, scf_dn_status_cmp);
			if (status && status->alias) {
				if (SCF_OP_DEREFERENCE == c->op->type) {
					aliases = scf_vector_alloc();
					if (!aliases)
						return -ENOMEM;

					ret = scf_vector_add(aliases, status->alias);
					if (ret < 0) {
						scf_vector_free(aliases);
						aliases = NULL;
						return ret;
					}
					break;
				}

				ret = _3ac_pointer_alias(status->alias, c);
				if (ret < 0)
					return ret;
				continue;
			}

			aliases = scf_vector_alloc();
			if (!aliases)
				return -ENOMEM;

			ret = _bb_pointer_aliases(aliases, bb_list_head, bb, dn_pointer);
			if (ret < 0) {
				scf_vector_free(aliases);
				aliases = NULL;
				return ret;
			}
			if (SCF_OP_DEREFERENCE == c->op->type)
				break;
			if (aliases->size > 1)
				break;

			ret = _3ac_pointer_alias(aliases->data[0], c);
			scf_vector_free(aliases);
			aliases = NULL;
			if (ret < 0)
				return ret;
		}

		if (l2 != scf_list_sentinel(&bb->code_list_head)) {

			scf_basic_block_t* bb_child  = NULL;
			scf_basic_block_t* bb_child2 = NULL;

			if (l2 != scf_list_head(&bb->code_list_head)) {
				ret = _bb_split(bb, &bb_child);
				if (ret < 0) {
					scf_vector_free(aliases);
					aliases = NULL;
					return ret;
				}

				ret = _bb_copy_aliases(bb_child, dn_pointer, aliases);
				scf_vector_free(aliases);
				aliases = NULL;
				if (ret < 0) {
					scf_basic_block_free(bb_child);
					bb_child = NULL;
					return ret;
				}

				while (l2 != scf_list_sentinel(&bb->code_list_head)) {
					c  = scf_list_data(l2, scf_3ac_code_t, list);
					l2 = scf_list_next(l2);

					scf_list_del(&c->list);
					scf_list_add_tail(&bb_child->code_list_head, &c->list);

					if (scf_type_is_assign(c->op->type)
							|| scf_type_is_assign_dereference(c->op->type))
						break;
				}

				bb_child->dereference_flag = 1;
				scf_list_add_front(&bb->list, &bb_child->list);
			} else {
				ret = _bb_copy_aliases(bb, dn_pointer, aliases);
				scf_vector_free(aliases);
				aliases = NULL;
				if (ret < 0)
					return ret;

				while (l2 != scf_list_sentinel(&bb->code_list_head)) {
					c  = scf_list_data(l2, scf_3ac_code_t, list);
					l2 = scf_list_next(l2);

					if (scf_type_is_assign(c->op->type)
							|| scf_type_is_assign_dereference(c->op->type))
						break;
				}

				bb_child = bb;
			}

			if (l2 != scf_list_sentinel(&bb->code_list_head)) {

				ret = _bb_split(bb_child, &bb_child2);
				if (ret < 0)
					return ret;

				while (l2 != scf_list_sentinel(&bb->code_list_head)) {
					c  = scf_list_data(l2, scf_3ac_code_t, list);
					l2 = scf_list_next(l2);

					scf_list_del(&c->list);
					scf_list_add_tail(&bb_child2->code_list_head, &c->list);
				}

				bb_child2->dereference_flag = 1;
				scf_list_add_front(&bb_child->list, &bb_child2->list);

				l = &bb_child2->list;
			}
		}
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

