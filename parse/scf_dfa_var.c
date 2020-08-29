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

int _var_add_var(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	scf_parse_t* parse = dfa->priv;

	if (d->current_identity) {

		scf_logi("d->current_identity: %p\n", d->current_identity);

		scf_variable_t* var = scf_scope_find_variable(parse->ast->current_block->scope, d->current_identity->text->data);
		if (var) {
			scf_loge("repeated declare var '%s', line: %d\n", d->current_identity->text->data, d->current_identity->line);
			return SCF_DFA_ERROR;
		}

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

			if (0 == d->nb_pointers && d->current_type->type >= SCF_STRUCT) {
				// if not pointer var, check if define recursive struct/union/class var

				if (_check_recursive((scf_type_t*)b, d->current_type, d->current_identity) < 0) {

					scf_loge("recursive define when define var '%s', line: %d\n",
							d->current_identity->text->data, d->current_identity->line);
					return SCF_DFA_ERROR;
				}
			}
		}

		if (SCF_FUNCTION_PTR == d->current_type->type
				&& (!d->func_ptr || 0 == d->nb_pointers)) {
			scf_loge("invalid func ptr\n");
			return SCF_DFA_ERROR;
		}

		var = SCF_VAR_ALLOC_BY_TYPE(d->current_identity, d->current_type, d->const_flag, d->nb_pointers, d->func_ptr);
		if (!var) {
			scf_loge("alloc var failed\n");
			return SCF_DFA_ERROR;
		}
		var->local_flag  = local_flag;
		var->global_flag = global_flag;
		var->member_flag = member_flag;

		scf_logi("type: %d, nb_pointers: %d,nb_dimentions: %d, var: %s,line:%d,pos:%d, local: %d, global: %d, member: %d\n\n",
				var->type, var->nb_pointers, var->nb_dimentions,
				var->w->text->data, var->w->line, var->w->pos,
				var->local_flag, var->global_flag, var->member_flag);

		scf_scope_push_var(parse->ast->current_block->scope, var);

		d->current_var      = var;
		d->current_identity = NULL;

		d->nb_pointers = 0;
		d->const_flag  = 0;
	}

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

	if (d->expr) {
		while(d->expr->parent)
			d->expr = d->expr->parent;

		scf_logi("d->expr: %p, d->expr->parent: %p\n", d->expr, d->expr->parent);

		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)d->expr);
		d->expr = NULL;
	}

	if (d->current_var)
		scf_variable_size(d->current_var);

	return SCF_DFA_SWITCH_TO;
}

static int _var_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (_var_add_var(dfa, d) < 0) {
		scf_loge("add var error\n");
		return SCF_DFA_ERROR;
	}

	d->nb_lss   = 0;
	d->nb_rss   = 0;
	d->func_ptr = NULL;

	if (d->expr) {
		while(d->expr->parent)
			d->expr = d->expr->parent;

		if (0 == d->expr->nb_nodes)
			scf_expr_free(d->expr);
		else
			scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)d->expr);
		d->expr = NULL;
	}

	if (d->current_var)
		scf_variable_size(d->current_var);

	scf_logw("d->expr: %p\n", d->expr);
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

	if (d->current_var->nb_dimentions > 0) {
		scf_logi("var array '%s' init, nb_dimentions: %d\n",
				d->current_var->w->text->data, d->current_var->nb_dimentions);
		return SCF_DFA_NEXT_WORD;
	}

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_ASSIGN);
	scf_node_t*     n0 = scf_node_alloc(w, op->type, NULL);
	n0->op = op;

	scf_node_t*     n1 = scf_node_alloc(NULL, d->current_var->type, d->current_var);
	scf_expr_t*     e  = scf_expr_alloc();

	scf_node_add_child(n0, n1);
	scf_expr_add_node(e, n0);

	d->expr         = e;

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

	SCF_DFA_GET_MODULE_NODE(dfa, create,    create,   create);
	SCF_DFA_GET_MODULE_NODE(dfa, create,    identity, create_id);
	SCF_DFA_GET_MODULE_NODE(dfa, create,    rp,       create_rp);

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

	// create class object
	scf_dfa_node_add_child(assign,    create);
	scf_dfa_node_add_child(create_id, semicolon);
	scf_dfa_node_add_child(create_rp, semicolon);

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

