#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_continue;

static int _continue_is_continue(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_CONTINUE == w->type;
}

static int _continue_action_continue(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];

	scf_node_t*       _continue = scf_node_alloc(w, SCF_OP_CONTINUE, NULL);
	if (!_continue) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _continue);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _continue);

	return SCF_DFA_NEXT_WORD;
}

static int _continue_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	return SCF_DFA_OK;
}

static int _dfa_init_module_continue(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, continue, semicolon, scf_dfa_is_semicolon,  _continue_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, continue, _continue, _continue_is_continue, _continue_action_continue);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_continue(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, continue,   semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, continue,   _continue, _continue);

	scf_dfa_node_add_child(_continue, semicolon);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_continue =
{
	.name        = "continue",
	.init_module = _dfa_init_module_continue,
	.init_syntax = _dfa_init_syntax_continue,
};

