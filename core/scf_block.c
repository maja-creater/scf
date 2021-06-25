#include"scf_block.h"
#include"scf_scope.h"

scf_block_t* scf_block_alloc(scf_lex_word_t* w)
{
	scf_block_t* b = calloc(1, sizeof(scf_block_t));
	assert(b);

	b->node.type = SCF_OP_BLOCK;

	if (w)
		b->node.w = scf_lex_word_clone(w);
	else
		b->node.w = NULL;

	b->scope = scf_scope_alloc(w, "block");

	return b;
}

scf_block_t* scf_block_alloc_cstr(const char* name)
{
	scf_block_t* b = calloc(1, sizeof(scf_block_t));
	assert(b);

	b->node.type = SCF_OP_BLOCK;
	b->name = scf_string_cstr(name);
	b->scope = scf_scope_alloc(NULL, name);

	return b;
}

void scf_block_end(scf_block_t* b, scf_lex_word_t* w)
{
	if (w)
		b->w_end = scf_lex_word_clone(w);
}

void scf_block_free(scf_block_t* b)
{
	assert(b);
	assert(b->scope);

	scf_scope_free(b->scope);
	b->scope = NULL;

	if (b->name) {
		scf_string_free(b->name);
		b->name = NULL;
	}

	if (b->w_end) {
		scf_lex_word_free(b->w_end);
		b->w_end = NULL;
	}

	scf_node_free((scf_node_t*)b);
}

scf_type_t*	scf_block_find_type(scf_block_t* b, const char* name)
{
	assert(b);
	while (b) {
//		printf("%s(),%d, ### b: %p, parent: %p\n", __func__, __LINE__, b, b->node.parent);
		if (SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type || b->node.type >= SCF_STRUCT) {

			if (b->scope) {
				scf_type_t* t = scf_scope_find_type(b->scope, name);
				if (t)
					return t;
			}
		}
		b = (scf_block_t*)(b->node.parent);
	}
	return NULL;
}

scf_type_t*	scf_block_find_type_type(scf_block_t* b, const int type)
{
	assert(b);
	while (b) {
		if (SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type || b->node.type >= SCF_STRUCT) {

			if (b->scope) {
				scf_type_t* t = scf_scope_find_type_type(b->scope, type);
				if (t)
					return t;
			}
		}
		b = (scf_block_t*)(b->node.parent);
	}
	return NULL;
}

scf_variable_t*	scf_block_find_variable(scf_block_t* b, const char* name)
{
	assert(b);
	while (b) {
		if (SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type || b->node.type >= SCF_STRUCT) {

			if (b->scope) {
				scf_variable_t* v = scf_scope_find_variable(b->scope, name);
				if (v)
					return v;
			}
		}
		b = (scf_block_t*)(b->node.parent);
	}
	return NULL;
}

scf_function_t*	scf_block_find_function(scf_block_t* b, const char* name)
{
	assert(b);
	while (b) {
		if (SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type || b->node.type >= SCF_STRUCT) {

			if (b->scope) {
				scf_function_t* f = scf_scope_find_function(b->scope, name);
				if (f)
					return f;
			}
		}

		b = (scf_block_t*)(b->node.parent);
	}
	return NULL;
}

scf_label_t* scf_block_find_label(scf_block_t* b, const char* name)
{
	assert(b);
	while (b) {
		if (SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type) {
			assert(b->scope);

			scf_label_t* l = scf_scope_find_label(b->scope, name);
			if (l)
				return l;
		}

		b = (scf_block_t*)(b->node.parent);
	}
	return NULL;
}
