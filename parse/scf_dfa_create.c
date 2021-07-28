#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_create;

typedef struct {

	int              nb_lps;
	int              nb_rps;

	scf_node_t*      create;

	scf_expr_t*      parent_expr;

} create_module_data_t;

static int _create_is_create(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w     = word;

	return SCF_LEX_WORD_KEY_CREATE == w->type;
}

static int _create_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t*     d     = data;
	scf_stack_t*          s     = d->module_datas[dfa_module_create.index];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	md->nb_lps++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _create_action_create(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	SCF_CHECK_ERROR(md->create, SCF_DFA_ERROR, "\n");

	md->create = scf_node_alloc(w, SCF_OP_CREATE, NULL);
	if (!md->create)
		return SCF_DFA_ERROR;

	md->nb_lps      = 0;
	md->nb_rps      = 0;
	md->parent_expr = d->expr;

	return SCF_DFA_NEXT_WORD;
}

static int _create_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	scf_type_t* t  = NULL;
	scf_type_t* pt = NULL;

	if (scf_ast_find_type(&t, parse->ast, w->text->data) < 0)
		return SCF_DFA_ERROR;

	if (!t) {
		scf_loge("type '%s' not found\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	if (scf_ast_find_type_type(&pt, parse->ast, SCF_FUNCTION_PTR) < 0)
		return SCF_DFA_ERROR;
	assert(pt);

	scf_variable_t* var = SCF_VAR_ALLOC_BY_TYPE(w, pt, 1, 1, NULL);
	SCF_CHECK_ERROR(!var, SCF_DFA_ERROR, "var '%s' alloc failed\n", w->text->data);
	var->const_literal_flag = 1;

	scf_node_t* node = scf_node_alloc(NULL, var->type, var);
	SCF_CHECK_ERROR(!node, SCF_DFA_ERROR, "node alloc failed\n");

	int ret = scf_node_add_child(md->create, node);
	SCF_CHECK_ERROR(ret < 0, SCF_DFA_ERROR, "node add child failed\n");

	w = dfa->ops->pop_word(dfa);
	if (SCF_LEX_WORD_LP != w->type) {

		if (d->expr) {
			ret = scf_expr_add_node(d->expr, md->create);
			SCF_CHECK_ERROR(ret < 0, SCF_DFA_ERROR, "expr add child failed\n");
		} else
			d->expr = md->create;

		md->create = NULL;
	}
	dfa->ops->push_word(dfa, w);

	return SCF_DFA_NEXT_WORD;
}

static int _create_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	scf_logd("d->expr: %p\n", d->expr);

	d->expr = NULL;
	d->expr_local_flag++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _create_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	md->nb_rps++;

	scf_logd("md->nb_lps: %d, md->nb_rps: %d\n", md->nb_lps, md->nb_rps);

	if (md->nb_rps < md->nb_lps) {

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_rp"),      SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_comma"),   SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_lp_stat"), SCF_DFA_HOOK_POST);

		return SCF_DFA_NEXT_WORD;
	}
	assert(md->nb_rps == md->nb_lps);

	if (d->expr) {
		int ret = scf_node_add_child(md->create, d->expr);
		d->expr = NULL;
		SCF_CHECK_ERROR(ret < 0, SCF_DFA_ERROR, "node add child failed\n");
	}

	d->expr = md->parent_expr;
	d->expr_local_flag--;

	if (d->expr) {
		int ret = scf_expr_add_node(d->expr, md->create);
		SCF_CHECK_ERROR(ret < 0, SCF_DFA_ERROR, "expr add child failed\n");
	} else
		d->expr = md->create;

	md->create = NULL;

	scf_logi("d->expr: %p\n", d->expr);

	return SCF_DFA_NEXT_WORD;
}

static int _create_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	SCF_CHECK_ERROR(!d->expr, SCF_DFA_ERROR, "\n");

	int ret = scf_node_add_child(md->create, d->expr);
	d->expr = NULL;
	SCF_CHECK_ERROR(ret < 0, SCF_DFA_ERROR, "node add child failed\n");

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "create_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _dfa_init_module_create(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, create, create,    _create_is_create,    _create_action_create);

	SCF_DFA_MODULE_NODE(dfa, create, identity,  scf_dfa_is_identity,  _create_action_identity);

	SCF_DFA_MODULE_NODE(dfa, create, lp,        scf_dfa_is_lp,        _create_action_lp);
	SCF_DFA_MODULE_NODE(dfa, create, rp,        scf_dfa_is_rp,        _create_action_rp);

	SCF_DFA_MODULE_NODE(dfa, create, lp_stat,   scf_dfa_is_lp,        _create_action_lp_stat);

	SCF_DFA_MODULE_NODE(dfa, create, comma,     scf_dfa_is_comma,     _create_action_comma);

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	assert(!md);

	md = calloc(1, sizeof(create_module_data_t));
	if (!md) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_create.index] = md;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_create(scf_dfa_t* dfa)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	create_module_data_t* md    = d->module_datas[dfa_module_create.index];

	if (md) {
		free(md);
		md = NULL;
		d->module_datas[dfa_module_create.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_create(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, create,  create,    create);
	SCF_DFA_GET_MODULE_NODE(dfa, create,  identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, create,  lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, create,  rp,        rp);
	SCF_DFA_GET_MODULE_NODE(dfa, create,  comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,    entry,     expr);

	scf_dfa_node_add_child(create,   identity);
	scf_dfa_node_add_child(identity, lp);

	scf_dfa_node_add_child(lp,       rp);

	scf_dfa_node_add_child(lp,       expr);
	scf_dfa_node_add_child(expr,     comma);
	scf_dfa_node_add_child(comma,    expr);
	scf_dfa_node_add_child(expr,     rp);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_create =
{
	.name        = "create",
	.init_module = _dfa_init_module_create,
	.init_syntax = _dfa_init_syntax_create,

	.fini_module = _dfa_fini_module_create,
};

