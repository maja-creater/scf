#include"scf_lex_word.h"

scf_lex_word_t*	scf_lex_word_alloc(scf_string_t* file, int line, int pos, int type)
{
	assert(file);

	scf_lex_word_t* w = calloc(1, sizeof(scf_lex_word_t));
	assert(w);

	w->file = scf_string_clone(file);
	assert(w->file);

	w->type = type;
	w->line = line;
	w->pos = pos;
	return w;
}

scf_lex_word_t*	scf_lex_word_clone(scf_lex_word_t* w)
{
	scf_lex_word_t* w1 = calloc(1, sizeof(scf_lex_word_t));
	assert(w1);

	scf_list_init(&w1->list);
	w1->type = w->type;

	switch (w->type) {
		case SCF_LEX_WORD_CONST_CHAR:
			w1->data.u32 = w->data.u32;
			break;
		case SCF_LEX_WORD_CONST_STRING:
			w1->data.s = scf_string_clone(w->data.s);
			break;

		case SCF_LEX_WORD_CONST_INT:
			w1->data.i = w->data.i;
			break;
		case SCF_LEX_WORD_CONST_FLOAT:
			w1->data.f = w->data.f;
			break;
		case SCF_LEX_WORD_CONST_DOUBLE:
			w1->data.d = w->data.d;
			break;
		case SCF_LEX_WORD_CONST_COMPLEX:
			w1->data.z = w->data.z;
			break;

		case SCF_LEX_WORD_CONST_I64:
			w1->data.i64 = w->data.i64;
			break;
		case SCF_LEX_WORD_CONST_U64:
			w1->data.u64 = w->data.u64;
			break;
		default:
			break;
	};

	w1->text = scf_string_clone(w->text);
	w1->file = scf_string_clone(w->file);
	w1->line = w->line;
	w1->pos = w->pos;

	return w1;
}

void scf_lex_word_free(scf_lex_word_t* w)
{
	assert(w);

	if (SCF_LEX_WORD_CONST_STRING == w->type)
		scf_string_free(w->data.s);

	if (w->text)
		scf_string_free(w->text);
	if (w->file)
		scf_string_free(w->file);

	free(w);
	w = NULL;
}

