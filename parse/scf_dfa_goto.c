#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_goto;

static int _goto_is_goto(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_GOTO == w->type;
}

static int _goto_action_goto(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];

	scf_node_t*       _goto  = scf_node_alloc(w, SCF_OP_GOTO, NULL);
	if (!_goto) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _goto);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _goto);

	d->current_goto = _goto;

	return SCF_DFA_NEXT_WORD;
}

static int _goto_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];

	scf_label_t*      l      = scf_label_alloc(w);
	scf_node_t*       n      = scf_node_alloc_label(l);

	scf_node_add_child(d->current_goto, n);

	return SCF_DFA_NEXT_WORD;
}

static int _goto_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d     = data;

	d->current_goto = NULL;

	return SCF_DFA_OK;
}

static int _dfa_init_module_goto(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, goto, _goto,     _goto_is_goto,        _goto_action_goto);
	SCF_DFA_MODULE_NODE(dfa, goto, identity,  scf_dfa_is_identity,  _goto_action_identity);
	SCF_DFA_MODULE_NODE(dfa, goto, semicolon, scf_dfa_is_semicolon, _goto_action_semicolon);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_goto(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, goto,   _goto,      _goto);
	SCF_DFA_GET_MODULE_NODE(dfa, goto,   identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, goto,   semicolon, semicolon);

	scf_dfa_node_add_child(_goto,    identity);
	scf_dfa_node_add_child(identity, semicolon);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_goto =
{
	.name        = "goto",
	.init_module = _dfa_init_module_goto,
	.init_syntax = _dfa_init_syntax_goto,
};

