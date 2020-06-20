#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_struct;

static int _struct_is__struct(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;
	return SCF_LEX_WORD_KEY_STRUCT == w->type;
}

static int _struct_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w1    = words->data[words->size - 1];
	scf_type_t*       t     = NULL;
	dfa_parse_data_t* d     = data;

	if (!d->root_struct) {
		t = scf_block_find_type(parse->ast->current_block, w1->text->data);
		if (!t) {
			t = scf_type_alloc(w1, w1->text->data, SCF_STRUCT + parse->ast->nb_structs++, 0);
			if (!t) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			scf_scope_push_type(parse->ast->current_block->scope, t);
		} else {
			if (t->type < SCF_STRUCT) {
				scf_loge("struct name '%s' is same to base type\n", w1->text->data);
				return SCF_DFA_ERROR;
			}
		}

		d->root_struct      = t;
		d->current_struct   = t;
		d->current_type = t;
		d->nb_lbs           = 0;
		d->nb_rbs           = 0;

		scf_logd("t: %p, d->current_type: %p\n", t, d->current_type);
	} else {
		if (!d->current_struct) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		if (!d->current_struct->scope) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		t = d->current_struct;
		while (t) {
			if (!strcmp(w1->text->data, t->name->data))
				break;
			t = t->parent;
		}

		if (!t) {
			t = scf_scope_find_type(d->current_struct->scope, w1->text->data);
			if (!t) {
				t = scf_type_alloc(w1, w1->text->data, SCF_STRUCT + parse->ast->nb_structs++, 0);
				if (!t) {
					printf("%s(), %d, error: \n", __func__, __LINE__);
					return SCF_DFA_ERROR;
				}

				scf_scope_push_type(d->current_struct->scope, t);

				t->parent = d->current_struct;
			}
		}

		d->current_type = t;
		scf_logd("t: %p, d->current_type: %p\n", t, d->current_type);
	}
	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_lb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t* parse = dfa->priv;

	if (words->size < 3) {
		printf("%s(), %d, words->size: %d\n", __func__, __LINE__, words->size);
		return SCF_DFA_ERROR;
	}

	scf_lex_word_t* w0 = words->data[words->size - 3];
	scf_lex_word_t* w1 = words->data[words->size - 2];
	scf_lex_word_t* w2 = words->data[words->size - 1];

	if (SCF_LEX_WORD_ASSIGN == w1->type)
		return SCF_DFA_NEXT_SYNTAX;

	assert(SCF_LEX_WORD_KEY_STRUCT == w0->type);

	scf_type_t*        t = NULL;
	dfa_parse_data_t*  d = data;

	if (0 == d->nb_lbs) {
		t = scf_block_find_type(parse->ast->current_block, w1->text->data);

	} else if (d->nb_lbs > 0) {
		t = scf_scope_find_type(d->current_struct->scope, w1->text->data);

	} else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!t) {
		scf_loge("type '%s' not found\n", w1->text->data);
		return SCF_DFA_ERROR;
	}

	if (t->scope) {
		scf_loge("repeated define struct '%s'\n", w1->text->data);
		return SCF_DFA_ERROR;
	}

	t->scope = scf_scope_alloc(w2, t->name->data);
	if (!t->scope) {
		scf_loge("scope alloc failed\n");
		return SCF_DFA_ERROR;
	}

	d->current_struct = t;
	d->nb_lbs++;

	scf_logi("t: %p, d->current_type: %p\n", t, d->current_type);

	return SCF_DFA_NEXT_WORD;
}

static scf_type_t* _struct_find_proper_type(scf_dfa_t* dfa, scf_type_t* s, int type)
{
	scf_parse_t* parse = dfa->priv;

	char* name = NULL;
	while (s) {
		assert(s->scope);

		scf_type_t* t = scf_scope_find_type_type(s->scope, type);
		if (!t)
			continue;

		name = t->name->data;
		if (t->scope) {
			return t;
		}

		s = s->parent;
	}

	scf_type_t* t = scf_block_find_type_type(parse->ast->current_block, type);
	if (!t)
		t = scf_block_find_type(parse->ast->current_block, name);

	return t;
}

static int _struct_calculate_size(scf_dfa_t* dfa, scf_type_t* s)
{
	scf_parse_t* parse = dfa->priv;

	int size = 0;
	int i;
	for (i = 0; i < s->scope->vars->size; i++) {

		scf_variable_t* v = s->scope->vars->data[i];

		if (v->size <= 0) {
			assert(0 == v->nb_pointers);

			if (v->type < SCF_STRUCT) {
				scf_loge("v: '%s'\n", v->w->text->data);
				return SCF_DFA_ERROR;
			}

			scf_type_t* t = _struct_find_proper_type(dfa, s, v->type);
			if (!t) {
				scf_loge("can't find proper type of v: '%s'\n", v->w->text->data);
				return SCF_DFA_ERROR;
			}

			v->type = t->type;
			v->size = t->size;
		}

		int align;
		if (1 == v->size)
			align = 1;
		else if (2 == v->size)
			align = 2;
		else if (v->size <= 4)
			align = 4;
		else
			align = 8;

		v->offset = (size + align - 1) / align * align;

		if (v->nb_dimentions > 0) {
			int j;
			int capacity = 1;

			for (j = 0; j < v->nb_dimentions; j++) {
				if (v->dimentions[j] < 0) {
					scf_loge("v: '%s'\n", v->w->text->data);
					return SCF_DFA_ERROR;

				} else if (0 == v->dimentions[j]) {

					if (j < v->nb_dimentions - 1) {
						scf_loge("v: '%s'\n", v->w->text->data);
						return SCF_DFA_ERROR;
					}
				}

				capacity *= v->dimentions[j];
			}

			v->capacity = capacity;
			size = v->offset + v->size * v->capacity;
		} else {
			size = v->offset + v->size;
		}

		scf_logi("struct '%s', member: '%s', offset: %d, size: %d, v->dim: %d, v->capacity: %d\n",
				s->name->data, v->w->text->data, v->offset, v->size, v->nb_dimentions, v->capacity);
	}
	s->size = size;

	scf_logi("struct '%s', size: %d\n", s->name->data, s->size);
	return 0;
}

static int _struct_action_rb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (_struct_calculate_size(dfa, d->current_struct) < 0) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->current_type = d->current_struct;
	d->current_struct   = d->current_struct->parent;
	d->nb_rbs++;
	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_ls(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (!d->current_var) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	assert(!d->expr);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "struct_rs"), SCF_DFA_HOOK_POST);

	d->nb_lss++;

	scf_variable_add_array_dimention(d->current_var, -1);

	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_rs(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;

	if (!d->current_var) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	while (d->expr->parent)
		d->expr = d->expr->parent;

	scf_variable_t* r = NULL;
	if (scf_expr_calculate(parse->ast, d->expr, &r) < 0) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	assert(d->nb_rss < d->current_var->nb_dimentions);
	d->current_var->dimentions[d->nb_rss] = r->data.i;

	scf_loge("d->expr: %p, r->data.i: %d\n", d->expr, r->data.i);

	d->nb_rss++;
	assert(d->nb_rss == d->nb_lss);

	scf_expr_free(d->expr);
	d->expr = NULL;

	scf_variable_free(r);
	r = NULL;

	return SCF_DFA_SWITCH_TO;
}

static int _struct_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->nb_pointers = 0;
	d->const_flag  = 0;

	if (d->nb_rbs == d->nb_lbs) {
		scf_logi("SCF_DFA_OK\n");

		d->root_struct      = NULL;
		d->current_struct   = NULL;
		d->current_type = NULL;
		d->nb_lbs           = 0;
		d->nb_rbs           = 0;
		return SCF_DFA_OK;

	} else if (d->nb_rbs > d->nb_lbs) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_logi("SCF_DFA_NEXT_WORD\n");
	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->nb_pointers = 0;
	d->const_flag  = 0;

	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_star(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->nb_pointers++;
	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_base_type(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];

	d->current_type  = scf_block_find_type(parse->ast->current_block, w->text->data);
	if (!d->current_type) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _struct_action_var(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];

	scf_logi("w: %s\n", w->text->data);

	scf_variable_t* var  = scf_scope_find_variable(d->current_struct->scope, w->text->data);
	if (var) {
		scf_loge("repeated declaration: %s\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (0 == d->nb_pointers) {
		if (d->nb_rbs < d->nb_lbs) {

			scf_type_t* t = d->current_struct;

			scf_logd("t: %p, d->current_type: %p\n", t, d->current_type);

			while (t) {
				if (t->type == d->current_type->type) {

					scf_loge("recursive member var declaration of struct type: %s, var name: %s\n",
							t->name->data, w->text->data);
					return SCF_DFA_ERROR;
				}
				t = t->parent;
			}
		}
	}

	var = SCF_VAR_ALLOC_BY_TYPE(w, d->current_type, 0, d->nb_pointers, NULL);
	if (!var) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}
	scf_scope_push_var(d->current_struct->scope, var);

	d->current_var = var;
	d->nb_lss      = 0;
	d->nb_rss      = 0;

	scf_logi("type: %d, nb_pointers: %d, var: %s, line: %d, pos: %d\n\n",
			var->type, var->nb_pointers,
			var->w->text->data, var->w->line, var->w->pos);

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_struct(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_ENTRY(dfa, struct);

	SCF_DFA_MODULE_NODE(dfa, struct, _struct,   _struct_is__struct,   NULL);
	SCF_DFA_MODULE_NODE(dfa, struct, lb,        scf_dfa_is_lb,        _struct_action_lb);
	SCF_DFA_MODULE_NODE(dfa, struct, rb,        scf_dfa_is_rb,        _struct_action_rb);
	SCF_DFA_MODULE_NODE(dfa, struct, identity,  scf_dfa_is_identity,  _struct_action_identity);
	SCF_DFA_MODULE_NODE(dfa, struct, semicolon, scf_dfa_is_semicolon, _struct_action_semicolon);

	SCF_DFA_MODULE_NODE(dfa, struct, base_type, scf_dfa_is_base_type, _struct_action_base_type);
	SCF_DFA_MODULE_NODE(dfa, struct, var,       scf_dfa_is_identity,  _struct_action_var);
	SCF_DFA_MODULE_NODE(dfa, struct, star,      scf_dfa_is_star,      _struct_action_star);
	SCF_DFA_MODULE_NODE(dfa, struct, comma,     scf_dfa_is_comma,     _struct_action_comma);

	SCF_DFA_MODULE_NODE(dfa, struct, ls,        scf_dfa_is_ls,        _struct_action_ls);
	SCF_DFA_MODULE_NODE(dfa, struct, rs,        scf_dfa_is_rs,        _struct_action_rs);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_struct(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, struct, entry,     entry);

	SCF_DFA_GET_MODULE_NODE(dfa, struct, _struct,   _struct);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, lb,        lb);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, rb,        rb);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, semicolon, semicolon);

	SCF_DFA_GET_MODULE_NODE(dfa, struct, base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, var,       var);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa, struct, ls,        ls);
	SCF_DFA_GET_MODULE_NODE(dfa, struct, rs,        rs);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,    entry,     expr);


	scf_vector_add(dfa->syntaxes, entry);
	scf_dfa_node_add_child(entry,     _struct);

	// struct start
	scf_dfa_node_add_child(_struct,   identity);

	// only struct declaration
	scf_dfa_node_add_child(identity,  semicolon);

	// struct define start
	scf_dfa_node_add_child(identity,  lb);

	// empty struct define
	scf_dfa_node_add_child(lb,        rb);

	// recursive internal struct define
	scf_dfa_node_add_child(lb,         _struct);

	// end current struct define, if root struct then end all struct define
	scf_dfa_node_add_child(rb,        semicolon);

	// recursive end struct define
	scf_dfa_node_add_child(semicolon, rb);

	// next internal struct define
	scf_dfa_node_add_child(semicolon, _struct);

	// internal base type member var declaration

	scf_dfa_node_add_child(lb,        base_type);
	// pointer var
	scf_dfa_node_add_child(base_type, star);

	// multi pointer var
	scf_dfa_node_add_child(star,      star);
	scf_dfa_node_add_child(star,      var);

	// normal var
	scf_dfa_node_add_child(base_type, var);

	// continuous var in same line: int a, b, *p, **pp, etc;
	scf_dfa_node_add_child(var,       comma);
	scf_dfa_node_add_child(comma,     star);
	scf_dfa_node_add_child(comma,     var);

	// array var
	scf_dfa_node_add_child(var,       ls);
	scf_dfa_node_add_child(ls,        expr);
	scf_dfa_node_add_child(expr,      rs);
	scf_dfa_node_add_child(rs,        ls);
	scf_dfa_node_add_child(rs,        semicolon);

	// end base type var define
	scf_dfa_node_add_child(var,       semicolon);

	// next internal base type member var
	scf_dfa_node_add_child(semicolon, base_type);

	// struct type member var declaration 
	scf_dfa_node_add_child(identity, star);
	scf_dfa_node_add_child(identity, var);
	scf_dfa_node_add_child(rb,       star);
	scf_dfa_node_add_child(rb,       var);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_struct = 
{
	.name        = "struct",
	.init_module = _dfa_init_module_struct,
	.init_syntax = _dfa_init_syntax_struct,
};

