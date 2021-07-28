#ifndef SCF_DFA_UTIL_H
#define SCF_DFA_UTIL_H

#include"scf_lex_word.h"
#include"scf_dfa.h"

static inline int scf_dfa_is_lp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LP == w->type;
}

static inline int scf_dfa_is_rp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RP == w->type;
}

static inline int scf_dfa_is_ls(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LS == w->type;
}

static inline int scf_dfa_is_rs(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RS == w->type;
}

static inline int scf_dfa_is_lb(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LB == w->type;
}

static inline int scf_dfa_is_rb(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RB == w->type;
}

static inline int scf_dfa_is_comma(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_COMMA == w->type;
}

static inline int scf_dfa_is_semicolon(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_SEMICOLON == w->type;
}

static inline int scf_dfa_is_star(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_STAR == w->type;
}

static inline int scf_dfa_is_identity(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return scf_lex_is_identity(w);
}

static inline int scf_dfa_is_base_type(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return scf_lex_is_base_type(w);
}

static inline int scf_dfa_is_sizeof(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_SIZEOF == w->type;
}

static inline int scf_dfa_is_vargs(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_VAR_ARGS == w->type;
}

static inline int scf_dfa_is_const(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_CONST == w->type;
}

static inline int scf_dfa_is_static(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_STATIC == w->type;
}

static inline int scf_dfa_is_extern(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_EXTERN == w->type;
}

static inline int scf_dfa_is_inline(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_INLINE == w->type;
}

static inline int scf_dfa_is_va_start(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_VA_START == w->type;
}

static inline int scf_dfa_is_va_arg(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_VA_ARG == w->type;
}

static inline int scf_dfa_is_va_end(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_VA_END == w->type;
}

static inline int scf_dfa_is_container(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_CONTAINER == w->type;
}

static inline int scf_dfa_is_const_string(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_CONST_STRING == w->type;
}

#endif

