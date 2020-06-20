#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_break;

static int _break_is_break(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_BREAK == w->type;
}

static int _break_action_break(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];

	scf_node_t*       _break = scf_node_alloc(w, SCF_OP_BREAK, NULL);
	if (!_break) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _break);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _break);

	return SCF_DFA_NEXT_WORD;
}

static int _break_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	return SCF_DFA_OK;
}

static int _dfa_init_module_break(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, break, semicolon, scf_dfa_is_semicolon, _break_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, break, _break,    _break_is_break,      _break_action_break);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_break(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, break,   semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, break,   _break,       _break);

	scf_dfa_node_add_child(_break, semicolon);

	printf("%s(),%d\n\n", __func__, __LINE__);
	return 0;
}

scf_dfa_module_t dfa_module_break =
{
	.name        = "break",
	.init_module = _dfa_init_module_break,
	.init_syntax = _dfa_init_syntax_break,
};

