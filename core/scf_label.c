#include"scf_node.h"

scf_label_t* scf_label_alloc(scf_lex_word_t* w)
{
	scf_label_t* l = calloc(1, sizeof(scf_label_t));
	assert(l);

	l->refs = 1;
	l->type = SCF_LABEL;
	l->w = scf_lex_word_clone(w);
	return l;
}

void scf_label_free(scf_label_t* l)
{
	assert(l);
	l->refs--;
	if (0 == l->refs) {
		if (l->w) {
			scf_lex_word_free(l->w);
			l->w = NULL;
		}

		l->node = NULL;
		free(l);
		l = NULL;
	}
}

