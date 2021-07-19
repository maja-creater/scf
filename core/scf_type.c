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

static scf_type_abbrev_t  type_abbrevs[] =
{
	{"int",       "i"},
	{"void",      "v"},

	{"char",      "c"},
	{"float",     "f"},
	{"double",    "d"},

	{"int8_t",    "b"},
	{"int16_t",   "w"},
	{"int32_t",   "i"},
	{"int64_t",   "q"},

	{"uint8_t",   "ub"},
	{"uint16_t",  "uw"},
	{"uint32_t",  "ui"},
	{"uint64_t",  "uq"},

	{"intptr_t",  "p"},
	{"uintptr_t", "up"},
	{"funcptr",   "fp"},

	{"string",    "s"},
};

const char* scf_type_find_abbrev(const char* name)
{
	scf_type_abbrev_t* t;

	int i;
	int n = sizeof(type_abbrevs) / sizeof(type_abbrevs[0]);

	for (i = 0; i < n; i++) {

		t  = &type_abbrevs[i];

		if (!strcmp(t->name, name))
			return t->abbrev;
	}

	return NULL;
}

