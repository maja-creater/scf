#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_identity;

static int _identity_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_lex_word_t*   w = words->data[words->size - 1];
	dfa_parse_data_t* d = data;
	scf_stack_t*      s = d->current_identities;

	scf_logd("w: '%s'\n", w->text->data);

	dfa_identity_t* id  = calloc(1, sizeof(dfa_identity_t));
	if (!id)
		return SCF_DFA_ERROR;

	if (scf_stack_push(s, id) < 0) {
		free(id);
		return SCF_DFA_ERROR;
	}

	id->identity = w;
	id->const_flag = d->const_flag;
	d ->const_flag = 0;

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_identity(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, identity, identity,  scf_dfa_is_identity,  _identity_action_identity);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_identity(scf_dfa_t* dfa)
{
	scf_logi("\n");

	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  identity);

	scf_vector_add(dfa->syntaxes, identity);

	return SCF_DFA_OK;
}

scf_dfa_module_t dfa_module_identity = 
{
	.name        = "identity",
	.init_module = _dfa_init_module_identity,
	.init_syntax = _dfa_init_syntax_identity,
};

