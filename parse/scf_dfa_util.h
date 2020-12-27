#ifndef SCF_DFA_UTIL_H
#define SCF_DFA_UTIL_H

#include"scf_lex_word.h"
#include"scf_dfa.h"

static int scf_dfa_is_lp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LP == w->type;
}

static int scf_dfa_is_rp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RP == w->type;
}

static int scf_dfa_is_ls(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LS == w->type;
}

static int scf_dfa_is_rs(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RS == w->type;
}

static int scf_dfa_is_lb(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LB == w->type;
}

static int scf_dfa_is_rb(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RB == w->type;
}

static int scf_dfa_is_comma(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_COMMA == w->type;
}

static int scf_dfa_is_semicolon(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_SEMICOLON == w->type;
}

static int scf_dfa_is_star(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_STAR == w->type;
}

static int scf_dfa_is_identity(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return scf_lex_is_identity(w);
}

static int scf_dfa_is_base_type(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return scf_lex_is_base_type(w);
}

static int scf_dfa_is_sizeof(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_SIZEOF == w->type;
}

static int scf_dfa_is_vargs(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_VAR_ARGS == w->type;
}

#endif

