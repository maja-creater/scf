#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t  dfa_module_va_arg;

int _type_find_type(scf_dfa_t* dfa, dfa_identity_t* id);

static int _va_arg_action_start(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	if (d->current_va_start
			|| d->current_va_arg
			|| d->current_va_end) {
		scf_loge("recursive 'va_start' in file: %s, line %d\n", w->file->data, w->line);
		return SCF_DFA_ERROR;
	}

	scf_node_t* node = scf_node_alloc(w, SCF_OP_VA_START, NULL);
	if (!node)
		return SCF_DFA_ERROR;

	scf_node_add_child((scf_node_t*)parse->ast->current_block, node);

	d->current_va_start = node;

	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_arg(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	if (d->current_va_start
			|| d->current_va_arg
			|| d->current_va_end) {
		scf_loge("recursive 'va_arg' in file: %s, line %d\n", w->file->data, w->line);
		return SCF_DFA_ERROR;
	}

	scf_node_t* node = scf_node_alloc(w, SCF_OP_VA_ARG, NULL);
	if (!node)
		return SCF_DFA_ERROR;

	if (d->expr)
		scf_expr_add_node(d->expr, node);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, node);

	d->current_va_arg = node;

	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	if (d->current_va_start
			|| d->current_va_arg
			|| d->current_va_end) {
		scf_loge("recursive 'va_end' in file: %s, line %d\n", w->file->data, w->line);
		return SCF_DFA_ERROR;
	}

	scf_node_t* node = scf_node_alloc(w, SCF_OP_VA_END, NULL);
	if (!node)
		return SCF_DFA_ERROR;

	scf_node_add_child((scf_node_t*)parse->ast->current_block, node);

	d->current_va_end = node;

	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	assert(d->current_va_start
			|| d->current_va_arg
			|| d->current_va_end);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "va_arg_rp"),         SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "va_arg_comma"),      SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_ap(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	scf_variable_t*    ap;
	scf_type_t*        t;
	scf_node_t*        node;

	ap = scf_block_find_variable(parse->ast->current_block, w->text->data);
	if (!ap) {
		scf_loge("va_list variable %s not found\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (scf_ast_find_type(&t, parse->ast, "va_list") < 0) {
		scf_loge("type 'va_list' not found, line: %d\n", w->line);
		return SCF_DFA_ERROR;
	}
	assert(t);

	if (t->type != ap->type || 0 != ap->nb_dimentions) {
		scf_loge("variable %s is not va_list type\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	node = scf_node_alloc(w, ap->type, ap);
	if (!node)
		return SCF_DFA_ERROR;

	if (d->current_va_start)
		scf_node_add_child(d->current_va_start, node);

	else if (d->current_va_arg)
		scf_node_add_child(d->current_va_arg, node);

	else if (d->current_va_end)
		scf_node_add_child(d->current_va_end, node);
	else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}
#if 0
	if (ap->arg_flag) {
		ap->nb_pointers = 1;
		ap->size        = sizeof(void*);
	}
#endif
	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "va_arg_comma"), SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _va_arg_action_fmt(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	scf_function_t*    f;
	scf_variable_t*    fmt;
	scf_variable_t*    arg;
	scf_node_t*        node;

	fmt = scf_block_find_variable(parse->ast->current_block, w->text->data);
	if (!fmt) {
		scf_loge("format string %s not found\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (SCF_VAR_CHAR != fmt->type
			&& SCF_VAR_I8 != fmt->type
			&& SCF_VAR_U8 != fmt->type) {

		scf_loge("format string %s is not 'char*' or 'int8*' or 'uint8*' type\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (scf_variable_nb_pointers(fmt) != 1) {
		scf_loge("format string %s is not 'char*' or 'int8*' or 'uint8*' type\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	f = (scf_function_t*)parse->ast->current_block;

	while (f && SCF_FUNCTION != f->node.type)
		f = (scf_function_t*) f->node.parent;

	if (!f) {
		scf_loge("va_list format string %s not in a function\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (!f->vargs_flag) {
		scf_loge("function %s has no variable args\n", f->node.w->text->data);
		return SCF_DFA_ERROR;
	}

	if (!f->argv || f->argv->size <= 0) {
		scf_loge("function %s with variable args should have one format string\n", f->node.w->text->data);
		return SCF_DFA_ERROR;
	}

	arg = f->argv->data[f->argv->size - 1];

	if (fmt != arg) {
		scf_loge("format string %s is not the last arg of function %s\n", w->text->data, f->node.w->text->data);
		return SCF_DFA_ERROR;
	}

	node = scf_node_alloc(w, fmt->type, fmt);
	if (!node)
		return SCF_DFA_ERROR;

	if (d->current_va_start)
		scf_node_add_child(d->current_va_start, node);
	else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _va_arg_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];

	if (d->current_va_arg) {

		if (d->current_va_arg->nb_nodes != 1) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		dfa_identity_t* id = scf_stack_pop(d->current_identities);
		if (!id) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		if (!id->type) {

			if (_type_find_type(dfa, id) < 0) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}
		}

		scf_variable_t* v = SCF_VAR_ALLOC_BY_TYPE(id->type_w, id->type, id->const_flag, id->nb_pointers, id->func_ptr);
		if (!v)
			return SCF_DFA_ERROR;

		scf_node_t* node = scf_node_alloc(w, v->type, v);
		if (!node)
			return SCF_DFA_ERROR;

		scf_node_add_child(d->current_va_arg, node);

		free(id);
		id = NULL;

		d->current_va_start = NULL;
		d->current_va_arg   = NULL;
		d->current_va_end   = NULL;

		return SCF_DFA_NEXT_WORD;
	}

	d->current_va_start = NULL;
	d->current_va_arg   = NULL;
	d->current_va_end   = NULL;

	return SCF_DFA_SWITCH_TO;
}

static int _va_arg_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	return SCF_DFA_OK;
}

static int _dfa_init_module_va_arg(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, va_arg, lp,         scf_dfa_is_lp,         _va_arg_action_lp);
	SCF_DFA_MODULE_NODE(dfa, va_arg, rp,         scf_dfa_is_rp,         _va_arg_action_rp);

	SCF_DFA_MODULE_NODE(dfa, va_arg, start,      scf_dfa_is_va_start,   _va_arg_action_start);
	SCF_DFA_MODULE_NODE(dfa, va_arg, arg,        scf_dfa_is_va_arg,     _va_arg_action_arg);
	SCF_DFA_MODULE_NODE(dfa, va_arg, end,        scf_dfa_is_va_end,     _va_arg_action_end);

	SCF_DFA_MODULE_NODE(dfa, va_arg, ap,         scf_dfa_is_identity,   _va_arg_action_ap);
	SCF_DFA_MODULE_NODE(dfa, va_arg, fmt,        scf_dfa_is_identity,   _va_arg_action_fmt);

	SCF_DFA_MODULE_NODE(dfa, va_arg, comma,      scf_dfa_is_comma,      _va_arg_action_comma);
	SCF_DFA_MODULE_NODE(dfa, va_arg, semicolon,  scf_dfa_is_semicolon,  _va_arg_action_semicolon);

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;

	d->current_va_start = NULL;
	d->current_va_arg   = NULL;
	d->current_va_end   = NULL;

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_va_arg(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   rp,        rp);

	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   start,     start);
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   arg,       arg);
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   ap,        ap);
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   fmt,       fmt);

	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa,   va_arg,   semicolon, semicolon);

	SCF_DFA_GET_MODULE_NODE(dfa,   type,     entry,     type);
	SCF_DFA_GET_MODULE_NODE(dfa,   type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa,   type,     star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa,   identity, identity,  identity);

	scf_dfa_node_add_child(start,     lp);
	scf_dfa_node_add_child(lp,        ap);
	scf_dfa_node_add_child(ap,        comma);
	scf_dfa_node_add_child(comma,     fmt);
	scf_dfa_node_add_child(fmt,       rp);
	scf_dfa_node_add_child(rp,        semicolon);

	scf_dfa_node_add_child(end,       lp);
	scf_dfa_node_add_child(ap,        rp);

	scf_dfa_node_add_child(arg,       lp);
	scf_dfa_node_add_child(ap,        type);

	scf_dfa_node_add_child(base_type, rp);
	scf_dfa_node_add_child(identity,  rp);
	scf_dfa_node_add_child(star,      rp);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_va_arg =
{
	.name        = "va_arg",
	.init_module = _dfa_init_module_va_arg,
	.init_syntax = _dfa_init_syntax_va_arg,
};

