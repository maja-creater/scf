#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_operator;

typedef struct {

	scf_block_t*     parent_block;

	scf_lex_word_t*  word_op;

	scf_dfa_hook_t*  hook_end;

} dfa_op_data_t;

static int _operator_is_key(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_OPERATOR == w->type;
}

static int _operator_is_op(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return (SCF_LEX_WORD_PLUS <= w->type && SCF_LEX_WORD_GE >= w->type);
}

int _operator_add_operator(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	scf_parse_t*     parse = dfa->priv;
	dfa_op_data_t*   opd   = d->module_datas[dfa_module_operator.index];
	dfa_identity_t*  id;
	scf_function_t*  f;
	scf_variable_t*  v;

	if (!opd->word_op) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	f = scf_function_alloc(opd->word_op);
	if (!f) {
		scf_loge("operator overloading function alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_logi("operator: %s,line:%d,pos:%d\n",
			f->node.w->text->data, f->node.w->line, f->node.w->pos);

	while (d->current_identities->size > 0) {

		id = scf_stack_pop(d->current_identities);

		if (!id || !id->type || !id->type_w) {
			scf_loge("function return value type NOT found\n");
			return SCF_DFA_ERROR;
		}

		f->static_flag |= id->static_flag;
		f->inline_flag |= id->inline_flag;

		v  = SCF_VAR_ALLOC_BY_TYPE(id->type_w, id->type, id->const_flag, id->nb_pointers, NULL);
		free(id);
		id = NULL;

		if (!v) {
			scf_function_free(f);
			return SCF_DFA_ERROR;
		}

		if (scf_vector_add(f->rets, v) < 0) {
			scf_variable_free(v);
			scf_function_free(f);
			return SCF_DFA_ERROR;
		}
	}

	int i;
	int j;
	for (i = 0; i < f->rets->size / 2;  i++) {
		j  =        f->rets->size - 1 - i;

		SCF_XCHG(f->rets->data[i], f->rets->data[j]);
	}

	opd->word_op = NULL;

	d->current_function = f;

	return SCF_DFA_NEXT_WORD;
}

int _operator_add_arg(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	scf_variable_t* arg = NULL;

	if (d->current_identities->size > 2) {
		scf_loge("operator parse args error\n");
		return SCF_DFA_ERROR;
	}

	if (2 == d->current_identities->size) {

		dfa_identity_t* id1 = scf_stack_pop(d->current_identities);
		dfa_identity_t* id0 = scf_stack_pop(d->current_identities);

		if (!id0 || !id0->type) {
			scf_loge("operator parse arg type error\n");
			return SCF_DFA_ERROR;
		}

		if (!id1 || !id1->identity) {
			scf_loge("operator parse arg name error\n");
			return SCF_DFA_ERROR;
		}

		arg = SCF_VAR_ALLOC_BY_TYPE(id1->identity, id0->type, id0->const_flag, id0->nb_pointers, id0->func_ptr);
		if (!arg) {
			scf_loge("arg var alloc failed\n");
			return SCF_DFA_ERROR;
		}

		scf_vector_add(d->current_function->argv, arg);
		scf_scope_push_var(d->current_function->scope, arg);
		arg->refs++;
		arg->arg_flag   = 1;
		arg->local_flag = 1;

		scf_logi("d->current_function->argv->size: %d, %p\n",
				d->current_function->argv->size, d->current_function);

		free(id0);
		free(id1);

		d->argc++;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _operator_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d   = data;

	if (_operator_add_arg(dfa, d) < 0) {
		scf_loge("add arg failed\n");
		return SCF_DFA_ERROR;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "operator_comma"), SCF_DFA_HOOK_PRE);

	return SCF_DFA_NEXT_WORD;
}

static int _operator_action_op(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_op_data_t*    opd   = d->module_datas[dfa_module_operator.index];

	opd->word_op = words->data[words->size - 1];

	return SCF_DFA_NEXT_WORD;
}

static int _operator_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_op_data_t*    opd   = d->module_datas[dfa_module_operator.index];

	assert(!d->current_node);

	if (_operator_add_operator(dfa, d) < 0) {
		scf_loge("add operator failed\n");
		return SCF_DFA_ERROR;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "operator_rp"), SCF_DFA_HOOK_PRE);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "operator_comma"), SCF_DFA_HOOK_PRE);

	d->argc = 0;
	d->nb_lps++;

	return SCF_DFA_NEXT_WORD;
}

static int _operator_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_op_data_t*    opd   = d->module_datas[dfa_module_operator.index];
	scf_function_t*   f     = NULL;

	d->nb_rps++;

	if (d->nb_rps < d->nb_lps) {
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "operator_rp"), SCF_DFA_HOOK_PRE);
		return SCF_DFA_NEXT_WORD;
	}

	if (_operator_add_arg(dfa, d) < 0) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (parse->ast->current_block->node.type >= SCF_STRUCT) {

		scf_type_t* t = (scf_type_t*)parse->ast->current_block;

		if (!t->node.class_flag) {
			scf_loge("only class has operator overloading\n");
			return SCF_DFA_ERROR;
		}

		assert(t->scope);

		f = scf_scope_find_same_function(t->scope, d->current_function);

	} else {
		scf_loge("only class has operator overloading\n");
		return SCF_DFA_ERROR;
	}

	if (f) {
		if (!f->node.define_flag) {
			int i;

			for (i = 0; i < f->argv->size; i++) {
				scf_variable_t* v0 = f->argv->data[i];
				scf_variable_t* v1 = d->current_function->argv->data[i];

				if (v1->w) {
					if (v0->w)
						scf_lex_word_free(v0->w);
					v0->w = scf_lex_word_clone(v1->w);
				}
			}

			scf_function_free(d->current_function);
			d->current_function = f;
		} else {
			scf_lex_word_t* w = dfa->ops->pop_word(dfa);

			if (SCF_LEX_WORD_SEMICOLON != w->type) {

				scf_loge("repeated define operator '%s', first in line: %d, second in line: %d\n",
						f->node.w->text->data, f->node.w->line, d->current_function->node.w->line); 

				dfa->ops->push_word(dfa, w);
				return SCF_DFA_ERROR;
			}

			dfa->ops->push_word(dfa, w);
		}
	} else {
		scf_operator_t* op = scf_find_base_operator(d->current_function->node.w->text->data, d->current_function->argv->size);
		if (!op) {
			scf_loge("operator: '%s', nb_operands: %d\n",
					d->current_function->node.w->text->data, d->current_function->argv->size);
			return SCF_DFA_ERROR;
		}

		d->current_function->op_type = op->type;

		scf_scope_push_operator(parse->ast->current_block->scope, d->current_function);
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)d->current_function);
	}

	opd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "operator_end"), SCF_DFA_HOOK_END);
	assert(opd->hook_end);

	opd->parent_block = parse->ast->current_block;
	parse->ast->current_block = (scf_block_t*)d->current_function;

	return SCF_DFA_NEXT_WORD;
}

static int _operator_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_op_data_t*   opd    = d->module_datas[dfa_module_operator.index];

	parse->ast->current_block  = (scf_block_t*)(opd->parent_block);

	if (d->current_function->node.nb_nodes > 0)
		d->current_function->node.define_flag = 1;

	opd->parent_block = NULL;
	opd->hook_end     = NULL;

	d->current_function = NULL;
	d->argc   = 0;
	d->nb_lps = 0;
	d->nb_rps = 0;

	scf_logi("\n");
	return SCF_DFA_OK;
}

static int _dfa_init_module_operator(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, operator, comma,  scf_dfa_is_comma, _operator_action_comma);
	SCF_DFA_MODULE_NODE(dfa, operator, end,    scf_dfa_is_entry, _operator_action_end);

	SCF_DFA_MODULE_NODE(dfa, operator, lp,     scf_dfa_is_lp,    _operator_action_lp);
	SCF_DFA_MODULE_NODE(dfa, operator, rp,     scf_dfa_is_rp,    _operator_action_rp);

	SCF_DFA_MODULE_NODE(dfa, operator, ls,     scf_dfa_is_ls,    _operator_action_op);
	SCF_DFA_MODULE_NODE(dfa, operator, rs,     scf_dfa_is_rs,    NULL);

	SCF_DFA_MODULE_NODE(dfa, operator, key,    _operator_is_key, NULL);
	SCF_DFA_MODULE_NODE(dfa, operator, op,     _operator_is_op,  _operator_action_op);

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	dfa_op_data_t*   opd    = d->module_datas[dfa_module_operator.index];

	assert(!opd);

	opd = calloc(1, sizeof(dfa_op_data_t));
	if (!opd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_operator.index] = opd;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_operator(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	dfa_op_data_t*   opd    = d->module_datas[dfa_module_operator.index];

	if (opd) {
		free(opd);
		opd = NULL;
		d->module_datas[dfa_module_operator.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_operator(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, operator, comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa, operator, end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa, operator, lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, operator, rp,        rp);

	SCF_DFA_GET_MODULE_NODE(dfa, operator, ls,        ls);
	SCF_DFA_GET_MODULE_NODE(dfa, operator, rs,        rs);

	SCF_DFA_GET_MODULE_NODE(dfa, operator, key,       key);
	SCF_DFA_GET_MODULE_NODE(dfa, operator, op,        op);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  type_name);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, block,    entry,     block);

	// operator start
	scf_dfa_node_add_child(base_type, key);
	scf_dfa_node_add_child(type_name, key);
	scf_dfa_node_add_child(star,      key);

	scf_dfa_node_add_child(key,       op);

	scf_dfa_node_add_child(key,       ls);
	scf_dfa_node_add_child(ls,        rs);

	scf_dfa_node_add_child(op,        lp);
	scf_dfa_node_add_child(rs,        lp);

	// operator args
	scf_dfa_node_add_child(lp,        base_type);
	scf_dfa_node_add_child(lp,        type_name);
	scf_dfa_node_add_child(lp,        rp);

	scf_dfa_node_add_child(identity,  comma);
	scf_dfa_node_add_child(identity,  rp);

	scf_dfa_node_add_child(base_type, comma);
	scf_dfa_node_add_child(type_name, comma);
	scf_dfa_node_add_child(base_type, rp);
	scf_dfa_node_add_child(type_name, rp);
	scf_dfa_node_add_child(star,      comma);
	scf_dfa_node_add_child(star,      rp);

	scf_dfa_node_add_child(comma,     base_type);
	scf_dfa_node_add_child(comma,     type_name);

	// operator body
	scf_dfa_node_add_child(rp,        block);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_operator =
{
	.name        = "operator",

	.init_module = _dfa_init_module_operator,
	.init_syntax = _dfa_init_syntax_operator,

	.fini_module = _dfa_fini_module_operator,
};

