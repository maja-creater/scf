#include"scf_scope.h"
#include"scf_type.h"

scf_type_t* scf_type_alloc(scf_lex_word_t* w, const char* name, int type, int size)
{
	scf_type_t* t = calloc(1, sizeof(scf_type_t));
	assert(t);

	t->type      = type;
	t->node.type = type;
	t->name      = scf_string_cstr(name);
//	printf("%s(),%d, t: %p, t->name: %p, t->name->data: %s, t->type: %d\n", __func__, __LINE__, t, t->name, t->name->data, t->type);

	if (w)
		t->w = scf_lex_word_clone(w);
	else
		t->w = NULL;

	t->size			= size;
	return t;
}

void scf_type_free(scf_type_t* t)
{
	assert(t);

	scf_string_free(t->name);
	t->name = NULL;

	if (t->w) {
		scf_lex_word_free(t->w);
		t->w = NULL;
	}

	if (t->scope) {
		scf_scope_free(t->scope);
		t->scope = NULL;
	}

	free(t);
	t = NULL;
}

