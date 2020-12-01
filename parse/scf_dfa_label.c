#include"scf_dfa.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_label;

static int _label_is_colon(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_COLON == w->type;
}

static int _label_action_colon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	dfa_identity_t*   id     = scf_stack_top(d->current_identities);

	if (!id || !id->identity) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_label_t* l = scf_label_alloc(id->identity);
	scf_node_t*  n = scf_node_alloc_label(l);

	scf_stack_pop(d->current_identities);
	free(id);
	id = NULL;

	scf_node_add_child((scf_node_t*)parse->ast->current_block, n);

	scf_scope_push_label(parse->ast->current_block->scope, l);

	return SCF_DFA_OK;
}

static int _dfa_init_module_label(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, label, label,  _label_is_colon, _label_action_colon);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_label(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, label,    label,    label);
	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity, identity);

	scf_dfa_node_add_child(identity, label);
	return 0;
}

scf_dfa_module_t dfa_module_label =
{
	.name        = "label",
	.init_module = _dfa_init_module_label,
	.init_syntax = _dfa_init_syntax_label,
};

