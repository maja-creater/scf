#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_return;

static int _return_is_return(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_RETURN == w->type;
}

static int _return_action_return(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse   = dfa->priv;
	dfa_parse_data_t* d       = data;
	scf_lex_word_t*   w       = words->data[words->size - 1];
	scf_node_t*       _return = NULL;

	if (d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	_return = scf_node_alloc(w, SCF_OP_RETURN, NULL);
	if (!_return) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _return);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _return);

	d->current_return  = _return;
	d->expr_local_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "return_semicolon"), SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "return_comma"),     SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _return_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (d->expr) {
		scf_node_add_child(d->current_return, d->expr);
		d->expr = NULL;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "return_comma"),     SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _return_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (d->expr) {
		scf_node_add_child(d->current_return, d->expr);
		d->expr = NULL;
	}

	d->expr_local_flag = 0;

	if (d->current_return->nb_nodes > 4) {
		scf_loge("return values must NOT more than 4!\n");
		return SCF_DFA_ERROR;
	}

	d->current_return  = NULL;

	return SCF_DFA_OK;
}

static int _dfa_init_module_return(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, return, semicolon, scf_dfa_is_semicolon, _return_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, return, comma,     scf_dfa_is_comma,     _return_action_comma);
	SCF_DFA_MODULE_NODE(dfa, return, _return,   _return_is_return,    _return_action_return);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_return(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, return,   semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, return,   comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa, return,   _return,   _return);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,     entry,     expr);

	scf_dfa_node_add_child(_return,    semicolon);
	scf_dfa_node_add_child(_return,    expr);
	scf_dfa_node_add_child(expr,       comma);
	scf_dfa_node_add_child(comma,      expr);
	scf_dfa_node_add_child(expr,       semicolon);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_return =
{
	.name        = "return",
	.init_module = _dfa_init_module_return,
	.init_syntax = _dfa_init_syntax_return,
};

