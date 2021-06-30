#include"scf_parse.h"

scf_expr_t* scf_expr_alloc()
{
	scf_expr_t* e = calloc(1, sizeof(scf_expr_t));
	if (!e) {
		scf_loge("expr alloc failed");
		return NULL;
	}

	e->nodes = calloc(1, sizeof(scf_node_t*));
	if (!e->nodes) {
		scf_loge("expr nodes alloc failed");

		free(e);
		e = NULL;
		return NULL;
	}

	e->type = SCF_OP_EXPR;
	return e;
}

void scf_expr_free(scf_expr_t* e)
{
	if (e) {
		scf_node_free(e);
		e = NULL;
	}
}

static int _scf_expr_node_add_node(scf_node_t** pparent, scf_node_t* child)
{
	scf_node_t* parent = *pparent;
	if (!parent) {
		*pparent = child;
		return 0;
	}

	if (parent->priority > child->priority) {
		assert(parent->op);

		if (parent->op->nb_operands > parent->nb_nodes)
			return scf_node_add_child(parent, child);
		else {
			assert(parent->nb_nodes >= 1);
			return _scf_expr_node_add_node(&(parent->nodes[parent->nb_nodes - 1]), child);
		}
	} else if (parent->priority < child->priority) {
		assert(child->op);

		if (child->op->nb_operands > 0)
			assert(child->op->nb_operands > child->nb_nodes);

		child->parent = parent->parent;
		if (scf_node_add_child(child, parent) < 0) {
			scf_loge("expr nodes alloc failed");
			return -1;
		}
		*pparent = child;
		return 0;
	} else {
		// parent->priority == child->priority
		assert(parent->op);
		assert(child->op);

		if (SCF_OP_ASSOCIATIVITY_LEFT == child->op->associativity) {
			if (child->op->nb_operands > 0)
				assert(child->op->nb_operands > child->nb_nodes);

			child->parent = parent->parent;
			scf_node_add_child(child, parent); // add parent to child's child node
			*pparent = child; // child is the new parent node
			return 0;
		} else {
			if (parent->op->nb_operands > parent->nb_nodes)
				return scf_node_add_child(parent, child);
			else {
				assert(parent->nb_nodes >= 1);
				return _scf_expr_node_add_node(&(parent->nodes[parent->nb_nodes - 1]), child);
			}
		}
	}

	scf_loge("\n");
	return -1;
}

int scf_expr_add_node(scf_expr_t* e, scf_node_t* node)
{
	assert(e);
	assert(node);

	if (scf_type_is_var(node->type)) {
		node->priority = -1;

	} else if (scf_type_is_operator(node->type)) {

		node->op = scf_find_base_operator_by_type(node->type);
		if (!node->op) {
			scf_loge("\n");
			return -1;
		}
		node->priority = node->op->priority;
	} else {
		scf_loge("\n");
		return -1;
	}

	if (_scf_expr_node_add_node(&(e->nodes[0]), node) < 0) {
		scf_loge("\n");
		return -1;
	}

	e->nodes[0]->parent = e;
	e->nb_nodes = 1;
	return 0;	
}

