#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_stack.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_expr;

typedef struct {
	scf_stack_t*      ls_exprs;
	scf_stack_t*      lp_exprs;
	scf_block_t*      parent_block;
	scf_variable_t*   current_var;
	scf_type_t*       current_struct;

} expr_module_data_t;

int _type_find_type(scf_dfa_t* dfa, dfa_identity_t* id);

static int _expr_is_expr(scf_dfa_t* dfa, void* word)
{
	scf_parse_t*    parse = dfa->priv;
	scf_lex_word_t* w     = word;

	if (SCF_LEX_WORD_SEMICOLON == w->type
			|| scf_lex_is_operator(w)
			|| scf_lex_is_const(w)
			|| scf_lex_is_identity(w))
		return 1;
	return 0;
}

static int _expr_is_number(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w     = word;

	return scf_lex_is_const(w);
}

static int _expr_is_unary_op(scf_dfa_t* dfa, void* word)
{
	scf_parse_t*    parse = dfa->priv;
	scf_lex_word_t* w     = word;

	if (SCF_LEX_WORD_LS == w->type
			|| SCF_LEX_WORD_RS == w->type
			|| SCF_LEX_WORD_LP == w->type
			|| SCF_LEX_WORD_RP == w->type
			|| SCF_LEX_WORD_KEY_SIZEOF == w->type)
		return 0;

	scf_operator_t* op = scf_find_base_operator(w->text->data, 1);
	if (op)
		return 1;
	return 0;
}

static int _expr_is_unary_post(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_INC == w->type
		|| SCF_LEX_WORD_DEC == w->type;
}

static int _expr_is_binary_op(scf_dfa_t* dfa, void* word)
{
	scf_parse_t*    parse = dfa->priv;
	scf_lex_word_t* w     = word;

	if (SCF_LEX_WORD_LS == w->type
			|| SCF_LEX_WORD_RS == w->type
			|| SCF_LEX_WORD_LP == w->type
			|| SCF_LEX_WORD_RP == w->type)
		return 0;

	scf_operator_t* op = scf_find_base_operator(w->text->data, 2);
	if (op)
		return 1;
	return 0;
}

int _expr_add_var(scf_parse_t* parse, dfa_parse_data_t* d)
{
	expr_module_data_t* md   = d->module_datas[dfa_module_expr.index];
	scf_variable_t*     var  = NULL;
	scf_node_t*         node = NULL;
	scf_type_t*         pt   = NULL;
	scf_function_t*     f    = NULL;
	dfa_identity_t*     id   = scf_stack_pop(d->current_identities);
	scf_lex_word_t*     w;

	assert(id && id->identity);
	w = id->identity;

	if (scf_ast_find_variable(&var, parse->ast, w->text->data) < 0)
		return SCF_DFA_ERROR;

	if (!var) {
		scf_logw("var '%s' not found, maybe it's a function\n", w->text->data);

		if (scf_ast_find_type_type(&pt, parse->ast, SCF_FUNCTION_PTR) < 0)
			return SCF_DFA_ERROR;
		assert(pt);

		if (scf_ast_find_function(&f, parse->ast, w->text->data) < 0)
			return SCF_DFA_ERROR;

		if (!f) {
			scf_loge("function '%s' not found\n", w->text->data);
			return SCF_DFA_ERROR;
		}

		var = SCF_VAR_ALLOC_BY_TYPE(id->identity, pt, 1, 1, f);
		if (!var) {
			scf_loge("var '%s' alloc failed\n", w->text->data);
			return SCF_DFA_ERROR;
		}
		var->const_literal_flag = 1;
	}

	node = scf_node_alloc(w, var->type, var);
	if (!node) {
		scf_loge("var node '%s' alloc failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (!d->expr) {
		d->expr = scf_expr_alloc();
		if (!d->expr) {
			scf_loge("expr alloc failed\n");
			return SCF_DFA_ERROR;
		}
	}

	scf_logd("d->expr: %p, node: %p\n", d->expr, node);

	if (scf_expr_add_node(d->expr, node) < 0) {
		scf_loge("add var node '%s' to expr failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (var->type >= SCF_STRUCT) {

		int ret = scf_ast_find_type_type(&md->current_struct, parse->ast, var->type);
		if (ret < 0)
			return SCF_DFA_ERROR;
		assert(md->current_struct);
	}
	md->current_var = var;

	free(id);
	id = NULL;
	return SCF_DFA_OK;
}

static int _expr_action_expr(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (!d->expr) {
		d->expr = scf_expr_alloc();
		if (!d->expr) {
			scf_loge("expr alloc failed\n");
			return SCF_DFA_ERROR;
		}
	}

	scf_logd("d->expr: %p\n", d->expr);

	return words->size > 0 ? SCF_DFA_CONTINUE : SCF_DFA_NEXT_WORD;
}

static int _expr_action_number(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];

	scf_logd("w: %s\n", w->text->data);

	int type;
	int nb_pointers = 0;

	switch (w->type) {
		case SCF_LEX_WORD_CONST_CHAR:
			type = SCF_VAR_U32;
			break;

		case SCF_LEX_WORD_CONST_STRING:
			type = SCF_VAR_CHAR;
			nb_pointers = 1;

			if (!strcmp(w->text->data, "__func__")) {

				scf_function_t* f = (scf_function_t*)parse->ast->current_block;

				while (f && SCF_FUNCTION != f->node.type)
					f = (scf_function_t*) f->node.parent;

				if (!f) {
					scf_loge("line: %d, '__func__' isn't in a function\n", w->line);
					return SCF_DFA_ERROR;
				}

				if (scf_function_signature(f) < 0)
					return SCF_DFA_ERROR;

				w->text->data[2] = '\0';
				w->text->len     = 2;

				int ret = scf_string_cat(w->text, f->signature);
				if (ret < 0)
					return SCF_DFA_ERROR;

				ret = scf_string_cat_cstr_len(w->text, "__", 2);
				if (ret < 0)
					return SCF_DFA_ERROR;

				w->data.s = scf_string_clone(f->signature);
				if (!w->data.s)
					return SCF_DFA_ERROR;
			}
			break;

		case SCF_LEX_WORD_CONST_INT:
			type = SCF_VAR_INT;
			break;
		case SCF_LEX_WORD_CONST_U32:
			type = SCF_VAR_U32;
			break;

		case SCF_LEX_WORD_CONST_FLOAT:
			type = SCF_VAR_FLOAT;
			break;

		case SCF_LEX_WORD_CONST_DOUBLE:
			type = SCF_VAR_DOUBLE;
			break;

		case SCF_LEX_WORD_CONST_I64:
			type = SCF_VAR_I64;
			break;

		case SCF_LEX_WORD_CONST_U64:
			type = SCF_VAR_U64;
			break;

		default:
			scf_loge("unknown number type\n");
			return SCF_DFA_ERROR;
	};

	scf_type_t* t = scf_block_find_type_type(parse->ast->current_block, type);
	if (!t) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_variable_t* var = SCF_VAR_ALLOC_BY_TYPE(w, t, 1, nb_pointers, NULL);
	if (!var) {
		scf_loge("var '%s' alloc failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	scf_node_t* n = scf_node_alloc(w, var->type, var);
	if (!n) {
		scf_loge("var node '%s' alloc failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (scf_expr_add_node(d->expr, n) < 0) {
		scf_loge("add var node '%s' to expr failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_op(scf_dfa_t* dfa, scf_vector_t* words, void* data, int nb_operands)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];

	scf_operator_t*   op;
	scf_node_t*       node;

	op = scf_find_base_operator(w->text->data, nb_operands);
	if (!op) {
		scf_loge("find op '%s' error, nb_operands: %d\n", w->text->data, nb_operands);
		return SCF_DFA_ERROR;
	}

	node = scf_node_alloc(w, op->type, NULL);
	if (!node) {
		scf_loge("op node '%s' alloc failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (scf_expr_add_node(d->expr, node) < 0) {
		scf_loge("add op node '%s' to expr failed\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_unary_op(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_logd("\n");
	return _expr_action_op(dfa, words, data, 1);
}

static int _expr_action_binary_op(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_logd("\n");

	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	scf_lex_word_t*     w     = words->data[words->size - 1];
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	dfa_identity_t*     id    = scf_stack_top(d->current_identities);

	if (SCF_LEX_WORD_STAR == w->type) {

		if (id && id->identity) {

			scf_variable_t* var = scf_block_find_variable(parse->ast->current_block,
					id->identity->text->data);

			if (!var) {
				scf_logw("'%s' not var\n", id->identity->text->data);

				if (d->expr) {
					scf_expr_free(d->expr);
					d->expr = NULL;
				}
				return SCF_DFA_NEXT_SYNTAX;
			}
		}
	}

	if (id && id->identity) {
		if (_expr_add_var(parse, d) < 0)
			return SCF_DFA_ERROR;
	}

	if (SCF_LEX_WORD_ARROW == w->type) {
		assert(md->current_struct);

		if (!md->parent_block)
			md->parent_block = parse->ast->current_block;

		parse->ast->current_block = (scf_block_t*)md->current_struct;

	} else if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block          = NULL;
	}

	return _expr_action_op(dfa, words, data, 2);
}

static int _expr_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	scf_expr_t* e = scf_expr_alloc();
	if (!e) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_logi("d->expr: %p, e: %p\n", d->expr, e);

	scf_stack_push(md->lp_exprs, d->expr);
	d->expr = e;

	if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block = NULL;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_rp_cast(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	dfa_identity_t*     id    = scf_stack_top(d->current_identities);

	if (!id) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!id->type) {
		if (_type_find_type(dfa, id) < 0) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}
	}

	if (!id->type || !id->type_w) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (d->nb_sizeofs > 0) {
		scf_logw("SCF_DFA_NEXT_SYNTAX\n");

		if (d->expr) {
			scf_expr_free(d->expr);
			d->expr = NULL;
		}

		return SCF_DFA_NEXT_SYNTAX;
	}

	if (d->current_va_arg) {
		scf_logw("SCF_DFA_NEXT_SYNTAX\n");
		return SCF_DFA_NEXT_SYNTAX;
	}

	scf_variable_t* var       = NULL;
	scf_node_t*     node_var  = NULL;
	scf_node_t*     node_cast = NULL;

	var = SCF_VAR_ALLOC_BY_TYPE(id->type_w, id->type, id->const_flag, id->nb_pointers, id->func_ptr);
	if (!var) {
		scf_loge("var alloc failed\n");
		return SCF_DFA_ERROR;
	}

	node_var = scf_node_alloc(NULL, var->type, var);
	if (!node_var) {
		scf_loge("var node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	node_cast = scf_node_alloc(id->type_w, SCF_OP_TYPE_CAST, NULL);
	if (!node_cast) {
		scf_loge("cast node alloc failed\n");
		return SCF_DFA_ERROR;
	}
	scf_node_add_child(node_cast, node_var);

	// '(' lp action pushed a expr before
	scf_expr_t* parent0 = scf_stack_pop(md->lp_exprs);
	scf_expr_t* parent1 = scf_stack_pop(md->lp_exprs);

	scf_logd("type cast 0: d->expr: %p, d->expr->parent: %p, 0: %p, 1: %p\n",
			d->expr, d->expr->parent, parent0, parent1);

	assert(parent0);

	if (parent1) {
		scf_expr_free(parent0);
		parent0 = NULL;

		scf_expr_add_node(parent1, node_cast);
		d->expr = parent1;

	} else {
		scf_expr_add_node(parent0, node_cast);
		d->expr = parent0;
	}

	scf_stack_pop(d->current_identities);
	free(id);
	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	dfa_identity_t*     id    = scf_stack_top(d->current_identities);

	if (id && id->identity) {

		scf_variable_t* v = NULL;
		scf_function_t* f = NULL;

		if (scf_ast_find_variable(&v, parse->ast, id->identity->text->data) < 0)
			return SCF_DFA_ERROR;

		if (!v) {
			scf_logw("'%s' not var\n", id->identity->text->data);

			if (scf_ast_find_function(&f, parse->ast, id->identity->text->data) < 0)
				return SCF_DFA_ERROR;

			if (!f) {
				scf_logw("'%s' not function\n", id->identity->text->data);
				return SCF_DFA_NEXT_SYNTAX;
			}
		}

		if (_expr_add_var(parse, d) < 0) {
			scf_loge("expr add var error\n");
			return SCF_DFA_ERROR;
		}
	}

	if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block          = NULL;
	}

	scf_expr_t* parent = scf_stack_pop(md->lp_exprs);

	scf_logd("d->expr: %p, d->expr->parent: %p, lp: %p\n\n", d->expr, d->expr->parent, parent);

	if (parent) {
		scf_expr_add_node(parent, d->expr);
		d->expr = parent;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_unary_post(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_parse_data_t* d     = data;
	scf_node_t*       n     = NULL;

	dfa_identity_t*   id    = scf_stack_top(d->current_identities);

	if (id && id->identity) {
		if (_expr_add_var(parse, d) < 0) {
			scf_loge("expr add var error\n");
			return SCF_DFA_ERROR;
		}
	}

	if (SCF_LEX_WORD_INC == w->type)
		n = scf_node_alloc(w, SCF_OP_INC_POST, NULL);

	else if (SCF_LEX_WORD_DEC == w->type)
		n = scf_node_alloc(w, SCF_OP_DEC_POST, NULL);
	else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!n) {
		scf_loge("node alloc error\n");
		return SCF_DFA_ERROR;
	}

	scf_expr_add_node(d->expr, n);

	scf_logd("n: %p, expr: %p, parent: %p\n", n, d->expr, d->expr->parent);

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_ls(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	scf_lex_word_t*     w     = words->data[words->size - 1];
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];
	dfa_identity_t*     id    = scf_stack_top(d->current_identities);

	if (id && id->identity) {
		if (_expr_add_var(parse, d) < 0) {
			scf_loge("expr add var error\n");
			return SCF_DFA_ERROR;
		}
	}

	if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block = NULL;
	}

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_ARRAY_INDEX);
	assert(op);

	scf_node_t* n = scf_node_alloc(w, op->type, NULL);
	if (!n) {
		scf_loge("node alloc error\n");
		return SCF_DFA_ERROR;
	}
	n->op = op;

	scf_expr_add_node(d->expr, n);

	scf_stack_push(md->ls_exprs, d->expr);

	scf_expr_t* index = scf_expr_alloc();
	if (!index) {
		scf_loge("index expr alloc error\n");
		return SCF_DFA_ERROR;
	}

	d->expr = index;

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_rs(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	dfa_identity_t*     id    = scf_stack_top(d->current_identities);

	if (id && id->identity) {
		if (_expr_add_var(parse, d) < 0) {
			scf_loge("expr add var error\n");
			return SCF_DFA_ERROR;
		}
	}

	if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block          = NULL;
	}

	scf_expr_t* ls_parent = scf_stack_pop(md->ls_exprs);

	assert (d->expr);
	while (d->expr->parent)
		d->expr = d->expr->parent;

	if (ls_parent) {
		scf_expr_add_node(ls_parent, d->expr);
		d->expr = ls_parent;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _expr_multi_rets(scf_expr_t* e)
{
	if (SCF_OP_ASSIGN != e->nodes[0]->type)
		return 0;

	scf_node_t*  parent = e->parent;
	scf_node_t*  assign = e->nodes[0];
	scf_node_t*  call   = assign->nodes[1];

	while (call) {
		if (SCF_OP_EXPR == call->type)
			call = call->nodes[0];
		else
			break;
	}

	int nb_rets;

	if (!call)
		return 0;
	else if (SCF_OP_CALL == call->type) {

		scf_variable_t* v_pf = _scf_operand_get(call->nodes[0]);
		scf_function_t* f    = v_pf->func_ptr;

		if (f->rets->size <= 1)
			return 0;

		nb_rets = f->rets->size;

	} else if (SCF_OP_CREATE == call->type)
		nb_rets = 2;
	else
		return 0;

	assert(call->nb_nodes > 0);

	scf_loge("parent->nb_nodes: %d\n", parent->nb_nodes);

	scf_node_t*  ret;
	scf_block_t* b;

	int i;
	int j;
	int k;

	b = scf_block_alloc_cstr("multi_rets");
	if (!b)
		return -ENOMEM;

	for (i  = parent->nb_nodes - 2; i >= 0; i--) {
		ret = parent->nodes[i];

		if (ret->semi_flag)
			break;

		if (b->node.nb_nodes >= nb_rets - 1)
			break;

		scf_node_add_child((scf_node_t*)b, ret);
		parent->nodes[i] = NULL;
	}

	j = 0;
	k = b->node.nb_nodes - 1;
	while (j < k) {
		SCF_XCHG(b->node.nodes[j++], b->node.nodes[k--]);
	}

	scf_node_add_child((scf_node_t*)b, assign->nodes[0]);
	assign->nodes[0] = (scf_node_t*)b;
	b->node.parent   = assign;

	i++;
	assert(i >= 0);

	parent->nodes[i] = e;
	parent->nb_nodes = i + 1;

	scf_loge("parent->nb_nodes: %d\n", parent->nb_nodes);

	return 0;
}

static int _expr_fini_expr(scf_parse_t* parse, dfa_parse_data_t* d, int semi_flag)
{
	expr_module_data_t* md = d->module_datas[dfa_module_expr.index];

	dfa_identity_t*     id = scf_stack_top(d->current_identities);

	if (id && id->identity) {
		if (_expr_add_var(parse, d) < 0)
			return SCF_DFA_ERROR;
	}

	if (md->parent_block) {
		parse->ast->current_block = md->parent_block;
		md->parent_block          = NULL;
	}

	scf_logd("d->expr: %p\n", d->expr);

	if (d->expr) {
		while (d->expr->parent)
			d->expr = d->expr->parent;

		if (0 == d->expr->nb_nodes) {
			scf_expr_free(d->expr);
			d->expr = NULL;

		} else if (!d->expr_local_flag) {

			scf_node_t* parent;

			if (d->current_node)
				parent = d->current_node;
			else
				parent = (scf_node_t*)parse->ast->current_block;

			scf_node_add_child(parent, d->expr);

			scf_loge("d->expr->parent->type: %d\n", d->expr->parent->type);

			if (_expr_multi_rets(d->expr) < 0) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			d->expr->semi_flag = semi_flag;
			d->expr = NULL;
		}
	}

	return SCF_DFA_OK;
}

static int _expr_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;

	if (_expr_fini_expr(parse, d, 0) < 0)
		return SCF_DFA_ERROR;

	return SCF_DFA_NEXT_WORD;
}

static int _expr_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	if (_expr_fini_expr(parse, d, 1) < 0)
		return SCF_DFA_ERROR;

	return SCF_DFA_OK;
}

static int _dfa_init_module_expr(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, expr, entry,      _expr_is_expr,        _expr_action_expr);
	SCF_DFA_MODULE_NODE(dfa, expr, number,     _expr_is_number,      _expr_action_number);
	SCF_DFA_MODULE_NODE(dfa, expr, unary_op,   _expr_is_unary_op,    _expr_action_unary_op);
	SCF_DFA_MODULE_NODE(dfa, expr, binary_op,  _expr_is_binary_op,   _expr_action_binary_op);
	SCF_DFA_MODULE_NODE(dfa, expr, unary_post, _expr_is_unary_post,  _expr_action_unary_post);

	SCF_DFA_MODULE_NODE(dfa, expr, lp,         scf_dfa_is_lp,        _expr_action_lp);
	SCF_DFA_MODULE_NODE(dfa, expr, rp,         scf_dfa_is_rp,        _expr_action_rp);
	SCF_DFA_MODULE_NODE(dfa, expr, rp_cast,    scf_dfa_is_rp,        _expr_action_rp_cast);

	SCF_DFA_MODULE_NODE(dfa, expr, ls,         scf_dfa_is_ls,        _expr_action_ls);
	SCF_DFA_MODULE_NODE(dfa, expr, rs,         scf_dfa_is_rs,        _expr_action_rs);

	SCF_DFA_MODULE_NODE(dfa, expr, comma,      scf_dfa_is_comma,     _expr_action_comma);
	SCF_DFA_MODULE_NODE(dfa, expr, semicolon,  scf_dfa_is_semicolon, _expr_action_semicolon);

	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = parse->dfa_data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	assert(!md);

	md = calloc(1, sizeof(expr_module_data_t));
	if (!md) {
		scf_loge("expr_module_data_t alloc error\n");
		return SCF_DFA_ERROR;
	}

	md->ls_exprs = scf_stack_alloc();
	if (!md->ls_exprs)
		goto _ls_exprs;

	md->lp_exprs = scf_stack_alloc();
	if (!md->lp_exprs)
		goto _lp_exprs;

	d->module_datas[dfa_module_expr.index] = md;

	return SCF_DFA_OK;

_lp_exprs:
	scf_stack_free(md->ls_exprs);
_ls_exprs:
	scf_loge("\n");

	free(md);
	md = NULL;
	return SCF_DFA_ERROR;
}

static int _dfa_fini_module_expr(scf_dfa_t* dfa)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = parse->dfa_data;
	expr_module_data_t* md    = d->module_datas[dfa_module_expr.index];

	if (md) {
		if (md->ls_exprs)
			scf_stack_free(md->ls_exprs);

		if (md->lp_exprs)
			scf_stack_free(md->lp_exprs);

		free(md);
		md = NULL;
		d->module_datas[dfa_module_expr.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_expr(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     entry,       expr);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     number,      number);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     unary_op,    unary_op);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     binary_op,   binary_op);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     unary_post,  unary_post);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,     lp,          lp);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     rp,          rp);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     rp_cast,     rp_cast);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,     ls,          ls);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     rs,          rs);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,     comma,       comma);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     semicolon,   semicolon);

	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,    identity);
	SCF_DFA_GET_MODULE_NODE(dfa, call,     lp,          call_lp);
	SCF_DFA_GET_MODULE_NODE(dfa, call,     rp,          call_rp);

	SCF_DFA_GET_MODULE_NODE(dfa, sizeof,   _sizeof,     _sizeof);
	SCF_DFA_GET_MODULE_NODE(dfa, sizeof,   rp,          sizeof_rp);

	SCF_DFA_GET_MODULE_NODE(dfa, create,   create,      create);
	SCF_DFA_GET_MODULE_NODE(dfa, create,   identity,    create_id);
	SCF_DFA_GET_MODULE_NODE(dfa, create,   rp,          create_rp);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     entry,       type_entry);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type,   base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,        star);

	SCF_DFA_GET_MODULE_NODE(dfa, va_arg,   arg,         va_arg);
	SCF_DFA_GET_MODULE_NODE(dfa, va_arg,   rp,          va_rp);

	SCF_DFA_GET_MODULE_NODE(dfa, container, container,  container);
	SCF_DFA_GET_MODULE_NODE(dfa, container, rp,         container_rp);

	// add expr to syntaxes
	scf_vector_add(dfa->syntaxes, expr);

	// expr start with number, identity, an unary_op, '(',
	// like: a = b, *p = 1, (a + b)
	// number start may be only useful in return statement.
	scf_dfa_node_add_child(expr,       number);
	scf_dfa_node_add_child(expr,       identity);
	scf_dfa_node_add_child(expr,       unary_op);
	scf_dfa_node_add_child(expr,       unary_post);
	scf_dfa_node_add_child(expr,       lp);
	scf_dfa_node_add_child(expr,       semicolon);

	// container(ptr, type, member)
	scf_dfa_node_add_child(expr,       container);
	scf_dfa_node_add_child(create_rp,  rp);
	scf_dfa_node_add_child(create_rp,  binary_op);
	scf_dfa_node_add_child(create_rp,  comma);
	scf_dfa_node_add_child(create_rp,  semicolon);

	// create class object
	scf_dfa_node_add_child(expr,       create);
	scf_dfa_node_add_child(create_id,  semicolon);
	scf_dfa_node_add_child(create_rp,  semicolon);

	// va_arg(ap, type)
	scf_dfa_node_add_child(expr,       va_arg);
	scf_dfa_node_add_child(va_rp,      rp);
	scf_dfa_node_add_child(va_rp,      binary_op);
	scf_dfa_node_add_child(va_rp,      comma);
	scf_dfa_node_add_child(va_rp,      semicolon);

	// sizeof()
	scf_dfa_node_add_child(expr,       _sizeof);
	scf_dfa_node_add_child(sizeof_rp,  rp);
	scf_dfa_node_add_child(sizeof_rp,  binary_op);
	scf_dfa_node_add_child(sizeof_rp,  comma);
	scf_dfa_node_add_child(sizeof_rp,  semicolon);

	// (expr)
	scf_dfa_node_add_child(lp,         identity);
	scf_dfa_node_add_child(lp,         number);
	scf_dfa_node_add_child(lp,         unary_op);
	scf_dfa_node_add_child(lp,         _sizeof);
	scf_dfa_node_add_child(lp,         lp);

	scf_dfa_node_add_child(identity,   rp);
	scf_dfa_node_add_child(number,     rp);
	scf_dfa_node_add_child(rp,         rp);

	scf_dfa_node_add_child(rp,         binary_op);
	scf_dfa_node_add_child(identity,   binary_op);
	scf_dfa_node_add_child(number,     binary_op);

	// type cast, like: (type*)var
	scf_dfa_node_add_child(lp,         type_entry);
	scf_dfa_node_add_child(base_type,  rp_cast);
	scf_dfa_node_add_child(star,       rp_cast);
	scf_dfa_node_add_child(identity,   rp_cast);

	scf_dfa_node_add_child(rp_cast,    identity);
	scf_dfa_node_add_child(rp_cast,    number);
	scf_dfa_node_add_child(rp_cast,    unary_op);
	scf_dfa_node_add_child(rp_cast,    _sizeof);
	scf_dfa_node_add_child(rp_cast,    lp);

	// identity() means function call, implement in scf_dfa_call.c
	scf_dfa_node_add_child(identity,   call_lp);
	scf_dfa_node_add_child(call_rp,    rp);
	scf_dfa_node_add_child(call_rp,    binary_op);
	scf_dfa_node_add_child(call_rp,    comma);
	scf_dfa_node_add_child(call_rp,    semicolon);

	// array index, a[1 + 2], a[]
	// [] is a special binary op,
	// should be added before normal binary op such as '+'
	scf_dfa_node_add_child(identity,   ls);
	scf_dfa_node_add_child(ls,         expr);
	scf_dfa_node_add_child(expr,       rs);
	scf_dfa_node_add_child(ls,         rs);
	scf_dfa_node_add_child(rs,         ls);
	scf_dfa_node_add_child(rs,         binary_op);

	scf_dfa_node_add_child(rs,         unary_post);
	scf_dfa_node_add_child(rs,         rp);
	scf_dfa_node_add_child(identity,   unary_post);

	// recursive unary_op, like: !*p
	scf_dfa_node_add_child(unary_op,   unary_op);
	scf_dfa_node_add_child(unary_op,   number);
	scf_dfa_node_add_child(unary_op,   identity);
	scf_dfa_node_add_child(unary_op,   expr);

	scf_dfa_node_add_child(binary_op,  unary_op);
	scf_dfa_node_add_child(binary_op,  number);
	scf_dfa_node_add_child(binary_op,  identity);

	// create class object
	scf_dfa_node_add_child(binary_op,  create);
	scf_dfa_node_add_child(create_id,  semicolon);
	scf_dfa_node_add_child(create_rp,  semicolon);

	scf_dfa_node_add_child(binary_op,  expr);

	scf_dfa_node_add_child(unary_post, rp);
	scf_dfa_node_add_child(unary_post, rs);
	scf_dfa_node_add_child(unary_post, binary_op);
	scf_dfa_node_add_child(unary_post, comma);
	scf_dfa_node_add_child(unary_post, semicolon);

	scf_dfa_node_add_child(rp,         comma);
	scf_dfa_node_add_child(number,     comma);
	scf_dfa_node_add_child(identity,   comma);
	scf_dfa_node_add_child(rs,         comma);
	scf_dfa_node_add_child(comma,      expr);

	scf_dfa_node_add_child(rp,         semicolon);
	scf_dfa_node_add_child(number,     semicolon);
	scf_dfa_node_add_child(identity,   semicolon);
	scf_dfa_node_add_child(rs,         semicolon);

	scf_logi("\n\n");
	return 0;
}

scf_dfa_module_t dfa_module_expr =
{
	.name        = "expr",

	.init_module = _dfa_init_module_expr,
	.init_syntax = _dfa_init_syntax_expr,

	.fini_module = _dfa_fini_module_expr,
};

