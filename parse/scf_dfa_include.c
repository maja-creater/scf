#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_include;

static int _include_is_include(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_INCLUDE == w->type;
}

static int _include_action_include(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];

	scf_loge("include '%s', line %d\n", w->text->data, w->line);
	return SCF_DFA_NEXT_WORD;
}

static int _include_action_path(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse  = dfa->priv;
	dfa_parse_data_t* d      = data;
	scf_lex_word_t*   w      = words->data[words->size - 1];
	scf_lex_t*        lex    = parse->lex;
	scf_block_t*      cur    = parse->ast->current_block;

	assert(w->data.s);
	scf_loge("include '%s', line %d\n", w->data.s->data, w->line);

	parse->lex = NULL;
	parse->ast->current_block = parse->ast->root_block;

	int ret = scf_parse_file(parse, w->data.s->data);
	if (ret < 0) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_lex_close(parse->lex);

	parse->lex = lex;
	parse->ast->current_block = cur;

	return SCF_DFA_NEXT_WORD;
}

static int _include_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	return SCF_DFA_OK;
}

static int _dfa_init_module_include(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, include, include,   _include_is_include,     _include_action_include);
	SCF_DFA_MODULE_NODE(dfa, include, path,      scf_dfa_is_const_string, _include_action_path);
	SCF_DFA_MODULE_NODE(dfa, include, semicolon, scf_dfa_is_semicolon,    _include_action_semicolon);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_include(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, include,   include,   include);
	SCF_DFA_GET_MODULE_NODE(dfa, include,   path,      path);
	SCF_DFA_GET_MODULE_NODE(dfa, include,   semicolon, semicolon);

	scf_vector_add(dfa->syntaxes,    include);

	scf_dfa_node_add_child(include,  path);
	scf_dfa_node_add_child(path,     semicolon);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_include =
{
	.name        = "include",
	.init_module = _dfa_init_module_include,
	.init_syntax = _dfa_init_syntax_include,
};

