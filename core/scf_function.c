#include"scf_function.h"
#include"scf_scope.h"
#include"scf_type.h"
#include"scf_block.h"

scf_function_t* scf_function_alloc(scf_lex_word_t* w)
{
	scf_function_t* f = calloc(1, sizeof(scf_function_t));
	assert(f);

	f->node.type = SCF_FUNCTION;

	assert(w);
	f->node.w = scf_lex_word_clone(w);

	f->scope = scf_scope_alloc(w, "function");

	f->argv           = scf_vector_alloc();
	f->jmps           = scf_vector_alloc();
	f->dfs_tree       = scf_vector_alloc();
	f->bb_loops       = scf_vector_alloc();

	f->text_relas     = scf_vector_alloc();
	f->data_relas     = scf_vector_alloc();

	f->op_type = -1;

	scf_list_init(&f->basic_block_list_head);
	scf_list_init(&f->dag_list_head);
	return f;
}

void scf_function_free(scf_function_t* f)
{
	assert(f);
	assert(f->scope);

	scf_scope_free(f->scope);
	f->scope = NULL;

	if (f->name) {
		scf_string_free(f->name);
		f->name = NULL;
	}

	if (f->w_start) {
		scf_lex_word_free(f->w_start);
		f->w_start = NULL;
	}

	if (f->w_end) {
		scf_lex_word_free(f->w_end);
		f->w_end = NULL;
	}

	int i;
	if (f->ret) {
		scf_variable_free(f->ret);
		f->ret = NULL;
	}

	if (f->argv) {
		for (i = 0; i < f->argv->size; i++) {
			scf_variable_free(f->argv->data[i]);
			f->argv->data[i] = NULL;
		}
		scf_vector_free(f->argv);
		f->argv = NULL;
	}

	if (f->jmps) {
		scf_vector_free(f->jmps);
		f->jmps = NULL;
	}

	if (f->text_relas) {
		scf_vector_free(f->text_relas);
		f->text_relas = NULL;
	}

	if (f->data_relas) {
		scf_vector_free(f->data_relas);
		f->data_relas = NULL;
	}

	scf_node_free((scf_node_t*)f);
}

int scf_function_same(scf_function_t* f0, scf_function_t* f1)
{
	if (strcmp(f0->node.w->text->data, f1->node.w->text->data))
		return 0;

	return scf_function_same_type(f0, f1);
}

int scf_function_same_argv(scf_vector_t* argv0, scf_vector_t* argv1)
{
	if (argv0) {
		if (!argv1)
			return 0;

		if (argv0->size != argv1->size)
			return 0;

		int i;
		for (i = 0; i < argv0->size; i++) {
			scf_variable_t* v0 = argv0->data[i];
			scf_variable_t* v1 = argv1->data[i];

			if (!scf_variable_type_like(v0, v1))
				return 0;
		}
	} else {
		if (argv1)
			return 0;
	}

	return 1;
}

int scf_function_same_type(scf_function_t* f0, scf_function_t* f1)
{
	if (f0->ret) {
		if (!f1->ret)
			return 0;

		if (!scf_variable_type_like(f0->ret, f1->ret))
			return 0;
	} else {
		if (f1->ret)
			return 0;
	}

	return scf_function_same_argv(f0->argv, f1->argv);
}

scf_string_t* scf_function_signature(scf_function_t* f)
{
	int ret;
	int i;

	scf_string_t* s = scf_string_alloc();
	if (!s)
		return NULL;

	scf_type_t*   t = (scf_type_t*)f->node.parent;
	if (t->node.type >= SCF_STRUCT) {
		assert(t->node.class_flag);

		ret = scf_string_cat(s, t->name);
		if (ret < 0)
			goto error;

		ret = scf_string_cat_cstr(s, "_");
		if (ret < 0)
			goto error;

	} else if (!strcmp(f->node.w->text->data, "main")) {
		ret = scf_string_cat_cstr(s, "main");
		if (ret < 0)
			goto error;
		return s;
	}

	ret = scf_string_cat(s, f->node.w->text);
	if (ret < 0)
		goto error;

	if (f->argv) {
		for (i = 0; i < f->argv->size; i++) {
			scf_variable_t* v   = f->argv->data[i];
			scf_type_t*     t_v = scf_block_find_type_type((scf_block_t*)t, v->type);

			ret = scf_string_cat_cstr(s, "_");
			if (ret < 0)
				goto error;

			ret = scf_string_cat(s, t_v->name);
			if (ret < 0)
				goto error;
		}
	}

	scf_logw("f signature: %s\n", s->data);

	return s;

error:
	scf_string_free(s);
	return NULL;
}

