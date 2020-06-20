#include"scf_dfa.h"
#include"scf_parse.h"

static void* dfa_pop_word(scf_dfa_t* dfa)
{
	scf_parse_t* parse = dfa->priv;

	scf_lex_word_t* w = NULL;
	scf_lex_pop_word(parse->lex, &w);
	return w;
}

static int dfa_push_word(scf_dfa_t* dfa, void* word)
{
	scf_parse_t* parse = dfa->priv;

	scf_lex_word_t* w = word;
	scf_lex_push_word(parse->lex, w);
	return 0;
}

static void dfa_free_word(void* word)
{
	scf_lex_word_t* w = word;
	scf_lex_word_free(w);
}
#if 0
static int dfa_same_type(void* word, scf_dfa_word_t* dfa_word)
{
	scf_lex_word_t* w = word;
	return w->type == dfa_word->type;
}
#endif
scf_dfa_ops_t dfa_ops_parse = 
{
	.name      = "parse",

	.pop_word  = dfa_pop_word,
	.push_word = dfa_push_word,
	.free_word = dfa_free_word,
//	.same_type = dfa_same_type,
};
