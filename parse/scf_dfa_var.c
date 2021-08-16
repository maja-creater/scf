#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_var;

static int _var_is_assign(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_ASSIGN == w->type;
}

int _check_recursive(scf_type_t* parent, scf_type_t* child, scf_lex_word_t* w)
{
	if (child->type == parent->type) {
	
		scf_loge("recursive define '%s' type member var '%s' in type '%s', line: %d\n",
				child->name->data, w->text->data, parent->name->data, w->line);

		return SCF_DFA_ERROR;
	}

	if (child->scope) {
		assert(child->type >= SCF_STRUCT);

		scf_variable_t* v      = NULL;
		scf_type_t*     type_v = NULL;
		int i;

		for (i = 0; i < child->scope->vars->size; i++) {

			v = child->scope->vars->data[i];
			if (v->nb_pointers > 0 || v->type < SCF_STRUCT)
				continue;

			type_v = scf_block_find_type_type((scf_block_t*)child, v->type);
			assert(type_v);

			if (_check_recursive(parent, type_v, v->w) < 0)
				return SCF_DFA_ERROR;
		}
	}

	return SCF_DFA_OK;
}

static int _var_add_var(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	scf_parse_t* parse = dfa->priv;

	dfa_identity_t* id = scf_stack_top(d->current_identities);

	if (id && id->identity) {

		scf_logd("d->current_identity: %p\n", id->identity);

		scf_variable_t* var = scf_scope_find_variable(parse->ast->current_block->scope, id->identity->text->data);
		if (var) {
			scf_loge("repeated declare var '%s', line: %d\n", id->identity->text->data, id->identity->line);
			return SCF_DFA_ERROR;
		}

		assert(d->current_identities->size >= 2);

		dfa_identity_t* id0 = d->current_identities->data[0];
		assert(id0 && id0->type);

		scf_block_t* b = parse->ast->current_block;
		while (b) {
			if (b->node.type >= SCF_STRUCT || SCF_FUNCTION == b->node.type)
				break;
			b = (scf_block_t*)b->node.parent;
		}

		uint32_t global_flag;
		uint32_t local_flag;
		uint32_t member_flag;

		if (!b) {
			local_flag  = 0;
			global_flag = 1;
			member_flag = 0;

		} else if (SCF_FUNCTION == b->node.type) {
			local_flag  = 1;
			global_flag = 0;
			member_flag = 0;

		} else if (b->node.type >= SCF_STRUCT) {
			local_flag  = 0;
			global_flag = 0;
			member_flag = 1;

			if (0 == id0->nb_pointers && id0->type->type >= SCF_STRUCT) {
				// if not pointer var, check if define recursive struct/union/class var

				if (_check_recursive((scf_type_t*)b, id0->type, id->identity) < 0) {

					scf_loge("recursive define when define var '%s', line: %d\n",
							id->identity->text->data, id->identity->line);
					return SCF_DFA_ERROR;
				}
			}
		}

		if (SCF_FUNCTION_PTR == id0->type->type
				&& (!id0->func_ptr || 0 == id0->nb_pointers)) {
			scf_loge("invalid func ptr\n");
			return SCF_DFA_ERROR;
		}

		if (id0->extern_flag) {
			if (!global_flag) {
				scf_loge("extern var must be global.\n");
				return SCF_DFA_ERROR;
			}

			scf_variable_t* v = scf_block_find_variable(parse->ast->current_block, id->identity->text->data);
			if (v) {
				scf_loge("extern var already declared, line: %d\n", v->w->line);
				return SCF_DFA_ERROR;
			}
		}

		if (SCF_VAR_VOID == id0->type->type && 0 == id0->nb_pointers) {
			scf_loge("void var must be a pointer, like void*\n");
			return SCF_DFA_ERROR;
		}

		var = SCF_VAR_ALLOC_BY_TYPE(id->identity, id0->type, id0->const_flag, id0->nb_pointers, id0->func_ptr);
		if (!var) {
			scf_loge("alloc var failed\n");
			return SCF_DFA_ERROR;
		}
		var->local_flag  = local_flag;
		var->global_flag = global_flag;
		var->member_flag = member_flag;

		var->static_flag = id0->static_flag;
		var->extern_flag = id0->extern_flag;

		scf_logi("type: %d, nb_pointers: %d,nb_dimentions: %d, var: %s,line:%d,pos:%d, local: %d, global: %d, member: %d, extern: %d, static: %d\n\n",
				var->type, var->nb_pointers, var->nb_dimentions,
				var->w->text->data, var->w->line, var->w->pos,
				var->local_flag,  var->global_flag, var->member_flag,
				var->extern_flag, var->static_flag);

		scf_scope_push_var(parse->ast->current_block->scope, var);

		d->current_var   = var;
		d->current_var_w = id->identity;
		id0->nb_pointers = 0;
		id0->const_flag  = 0;
		id0->static_flag = 0;
		id0->extern_flag = 0;

		scf_stack_pop(d->current_identities);
		free(id);
		id = NULL;
	}

	return 0;
}

static int _var_init_expr(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	scf_parse_t* parse = dfa->priv;

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	assert(d->current_var);

	d->expr_local_flag--;

	if (d->current_var->global_flag) {

		scf_logw("d->expr: %p, d->current_var: %p, global_flag: %d\n",
				d->expr, d->current_var, d->current_var->global_flag);

		if (scf_expr_calculate(parse->ast, d->expr, NULL) < 0) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		scf_expr_free(d->expr);

	} else {
		assert(d->expr->nb_nodes > 0);

		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)d->expr);
	}

	d->expr = NULL;
	return 0;
}

static int _var_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (_var_add_var(dfa, d) < 0) {
		scf_loge("add var error\n");
		return SCF_DFA_ERROR;
	}

	d->nb_lss = 0;
	d->nb_rss = 0;

	if (d->current_var)
		scf_variable_size(d->current_var);

	if (d->expr_local_flag > 0 && _var_init_expr(dfa, d) < 0)
		return SCF_DFA_ERROR;

	return SCF_DFA_SWITCH_TO;
}

static int _var_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_identity_t*   id    = NULL;

	if (_var_add_var(dfa, d) < 0) {
		scf_loge("add var error\n");
		return SCF_DFA_ERROR;
	}

	d->nb_lss   = 0;
	d->nb_rss   = 0;

	id = scf_stack_pop(d->current_identities);
	assert(id && id->type);
	free(id);
	id = NULL;

	if (d->current_var)
		scf_variable_size(d->current_var);


	if (d->expr_local_flag > 0) {

		if (_var_init_expr(dfa, d) < 0)
			return SCF_DFA_ERROR;

	} else if (d->expr) {
		scf_expr_free(d->expr);
		d->expr = NULL;
	}

	scf_node_t* b = (scf_node_t*)parse->ast->current_block;

	if (b->nb_nodes > 0)
		b->nodes[b->nb_nodes - 1]->semi_flag = 1;

	return SCF_DFA_OK;
}

static int _var_action_assign(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (_var_add_var(dfa, d) < 0) {
		scf_loge("add var error\n");
		return SCF_DFA_ERROR;
	}

	d->nb_lss = 0;
	d->nb_rss = 0;

	scf_lex_word_t*   w = words->data[words->size - 1];

	if (!d->current_var) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_var->extern_flag) {
		scf_loge("extern var '%s' can't be inited here, line: %d\n",
				d->current_var->w->text->data, w->line);
		return SCF_DFA_ERROR;
	}

	if (d->current_var->nb_dimentions > 0) {
		scf_logi("var array '%s' init, nb_dimentions: %d\n",
				d->current_var->w->text->data, d->current_var->nb_dimentions);
		return SCF_DFA_NEXT_WORD;
	}

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_ASSIGN);
	scf_node_t*     n0 = scf_node_alloc(w, op->type, NULL);
	n0->op = op;

	scf_node_t*     n1 = scf_node_alloc(d->current_var_w, d->current_var->type, d->current_var);
	scf_expr_t*     e  = scf_expr_alloc();

	scf_node_add_child(n0, n1);
	scf_expr_add_node(e, n0);

	d->expr         = e;
	d->expr_local_flag++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "var_semicolon"), SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "var_comma"), SCF_DFA_HOOK_POST);

	scf_logd("d->expr: %p\n", d->expr);

	return SCF_DFA_NEXT_WORD;
}

static int _var_action_ls(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (_var_add_var(dfa, d) < 0) {
		scf_loge("add var error\n");
		return SCF_DFA_ERROR;
	}

	d->nb_lss = 0;
	d->nb_rss = 0;

	if (!d->current_var) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	assert(!d->expr);
	scf_variable_add_array_dimention(d->current_var, -1);
	d->current_var->const_literal_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "var_rs"), SCF_DFA_HOOK_POST);
	//SCF_DFA_PUSH_END_WORD(SCF_LEX_WORD_RS, var, 0);

	d->nb_lss++;

	return SCF_DFA_NEXT_WORD;
}

static int _var_action_rs(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	d->nb_rss++;

	scf_logd("d->expr: %p\n", d->expr);

	if (d->expr) {
		while(d->expr->parent)
			d->expr = d->expr->parent;

		scf_logd("d->expr: %p, d->expr->parent: %p\n", d->expr, d->expr->parent);

		scf_variable_t* r = NULL;
		if (scf_expr_calculate(parse->ast, d->expr, &r) < 0) {
			scf_loge("scf_expr_calculate\n");
			return SCF_DFA_ERROR;
		}

		assert(d->current_var->dim_index < d->current_var->nb_dimentions);
		d->current_var->dimentions[d->current_var->dim_index++] = r->data.i;

		scf_logi("dimentions: %d, size: %d\n",
				d->current_var->dim_index, d->current_var->dimentions[d->current_var->dim_index - 1]);

		scf_variable_free(r);
		r = NULL;

		scf_expr_free(d->expr);
		d->expr = NULL;
	}

	return SCF_DFA_SWITCH_TO;
}

static int _dfa_init_module_var(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, var, comma,     scf_dfa_is_comma,     _var_action_comma);
	SCF_DFA_MODULE_NODE(dfa, var, semicolon, scf_dfa_is_semicolon, _var_action_semicolon);

	SCF_DFA_MODULE_NODE(dfa, var, ls,        scf_dfa_is_ls,        _var_action_ls);
	SCF_DFA_MODULE_NODE(dfa, var, rs,        scf_dfa_is_rs,        _var_action_rs);

	SCF_DFA_MODULE_NODE(dfa, var, assign,    _var_is_assign,       _var_action_assign);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_var(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, var,    comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa, var,    semicolon, semicolon);

	SCF_DFA_GET_MODULE_NODE(dfa, var,    ls,        ls);
	SCF_DFA_GET_MODULE_NODE(dfa, var,    rs,        rs);
	SCF_DFA_GET_MODULE_NODE(dfa, var,    assign,    assign);

	SCF_DFA_GET_MODULE_NODE(dfa, type,   star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, type,   identity,  identity);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,   entry,     expr);

	SCF_DFA_GET_MODULE_NODE(dfa, init_data, lb,     init_data_lb);
	SCF_DFA_GET_MODULE_NODE(dfa, init_data, rb,     init_data_rb);


	scf_dfa_node_add_child(identity,  comma);
	scf_dfa_node_add_child(comma,     star);
	scf_dfa_node_add_child(comma,     identity);

	// array var
	scf_dfa_node_add_child(identity,  ls);
	scf_dfa_node_add_child(ls,        rs);
	scf_dfa_node_add_child(ls,        expr);
	scf_dfa_node_add_child(expr,      rs);
	scf_dfa_node_add_child(rs,        ls);
	scf_dfa_node_add_child(rs,        comma);
	scf_dfa_node_add_child(rs,        semicolon);

	// var init
	scf_dfa_node_add_child(rs,        assign);
	scf_dfa_node_add_child(identity,  assign);

	// normal var init
	scf_dfa_node_add_child(assign,    expr);
	scf_dfa_node_add_child(expr,      comma);
	scf_dfa_node_add_child(expr,      semicolon);

	// struct or array var init
	scf_dfa_node_add_child(assign,       init_data_lb);
	scf_dfa_node_add_child(init_data_rb, comma);
	scf_dfa_node_add_child(init_data_rb, semicolon);

	scf_dfa_node_add_child(identity,  semicolon);

	printf("%s(),%d\n\n", __func__, __LINE__);
	return 0;
}

scf_dfa_module_t dfa_module_var = 
{
	.name        = "var",
	.init_module = _dfa_init_module_var,
	.init_syntax = _dfa_init_syntax_var,
};

