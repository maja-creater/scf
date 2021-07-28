#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_error;

typedef struct {
	scf_node_t* error;

} error_module_data_t;

static int _error_is_error(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_ERROR == w->type;
}

static int _error_action_error(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*         parse = dfa->priv;
	dfa_parse_data_t*    d     = data;
	error_module_data_t* md    = d->module_datas[dfa_module_error.index];
	scf_lex_word_t*      w     = words->data[words->size - 1];

	if (d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_node_t* error = scf_node_alloc(w, SCF_OP_ERROR, NULL);
	if (!error) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, error);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, error);

	md->error = error;
	d->expr_local_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "error_semicolon"), SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "error_comma"),     SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _error_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*         parse = dfa->priv;
	dfa_parse_data_t*    d     = data;
	error_module_data_t* md    = d->module_datas[dfa_module_error.index];

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_node_add_child(md->error, d->expr);
	d->expr = NULL;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "error_comma"),     SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _error_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*         parse = dfa->priv;
	dfa_parse_data_t*    d     = data;
	error_module_data_t* md    = d->module_datas[dfa_module_error.index];

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_node_add_child(md->error, d->expr);
	d->expr = NULL;

	if (md->error->nb_nodes < 3) {

		scf_type_t* t = NULL;

		if (scf_ast_find_type_type(&t, parse->ast, SCF_VAR_INT) < 0)
			return SCF_DFA_ERROR;
		assert(t);

		scf_expr_t* e = scf_expr_alloc();

		scf_variable_t* var = SCF_VAR_ALLOC_BY_TYPE(md->error->w, t, 1, 0, NULL);
		SCF_CHECK_ERROR(!var, SCF_DFA_ERROR, "add default error code -1 failed\n");
		var->const_literal_flag = 1;
		var->data.i = -1;

		scf_node_t* node = scf_node_alloc(NULL, var->type, var);
		SCF_CHECK_ERROR(!node, SCF_DFA_ERROR, "add default error code -1 failed\n");

		scf_expr_add_node(e, node);
		scf_node_add_child(md->error, e);
		e = NULL;
	}

	d->expr_local_flag = 0;
	md->error = NULL;

	return SCF_DFA_OK;
}

static int _dfa_init_module_error(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, error, semicolon, scf_dfa_is_semicolon, _error_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, error, comma,     scf_dfa_is_comma,     _error_action_comma);
	SCF_DFA_MODULE_NODE(dfa, error, error,     _error_is_error,      _error_action_error);

	scf_parse_t*         parse = dfa->priv;
	dfa_parse_data_t*    d     = parse->dfa_data;
	error_module_data_t* md    = d->module_datas[dfa_module_error.index];

	assert(!md);

	md = calloc(1, sizeof(error_module_data_t));
	if (!md) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_error.index] = md;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_error(scf_dfa_t* dfa)
{
	scf_parse_t*         parse = dfa->priv;
	dfa_parse_data_t*    d     = parse->dfa_data;
	error_module_data_t* md    = d->module_datas[dfa_module_error.index];

	if (md) {
		free(md);
		md = NULL;
		d->module_datas[dfa_module_error.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_error(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, error,   semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, error,   comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa, error,   error,     error);
	SCF_DFA_GET_MODULE_NODE(dfa, expr,    entry,     expr);

	scf_dfa_node_add_child(error,    expr);
	scf_dfa_node_add_child(expr,     comma);
	scf_dfa_node_add_child(comma,    expr);
	scf_dfa_node_add_child(expr,     semicolon);

	scf_loge("\n");
	return 0;
}

scf_dfa_module_t dfa_module_error =
{
	.name        = "error",
	.init_module = _dfa_init_module_error,
	.init_syntax = _dfa_init_syntax_error,

	.fini_module = _dfa_fini_module_error,
};

