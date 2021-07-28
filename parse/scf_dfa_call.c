#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_call;

typedef struct {

	int              nb_lps;
	int              nb_rps;

	scf_node_t*      func;
	scf_node_t*      call;
	scf_vector_t*    argv;

	scf_expr_t*      parent_expr;

} dfa_call_data_t;

static int _call_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t*  d     = data;
	scf_stack_t*       s     = d->module_datas[dfa_module_call.index];
	dfa_call_data_t*   cd    = scf_stack_top(s);

	if (!cd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	cd->nb_lps++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}


static int _call_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse     = dfa->priv;
	dfa_parse_data_t*  d         = data;
	scf_lex_word_t*    w1        = words->data[words->size - 1];
	scf_stack_t*       s         = d->module_datas[dfa_module_call.index];
	scf_function_t*    f         = NULL;
	dfa_call_data_t*   cd        = NULL;

	scf_variable_t*    var_pf    = NULL;
	scf_node_t*        node_pf   = NULL;
	scf_type_t*        pt        = NULL;

	scf_node_t*        node_call = NULL;
	scf_operator_t*    op        = scf_find_base_operator_by_type(SCF_OP_CALL);

	if (scf_ast_find_type_type(&pt, parse->ast, SCF_FUNCTION_PTR) < 0)
		return SCF_DFA_ERROR;

	assert(pt);
	assert(op);

	dfa_identity_t* id = scf_stack_top(d->current_identities);
	if (id && id->identity) {

		int ret = scf_ast_find_function(&f, parse->ast, id->identity->text->data);
		if (ret < 0)
			return SCF_DFA_ERROR;

		if (f) {
			scf_logd("f: %p, %s\n", f, f->node.w->text->data);

			var_pf = SCF_VAR_ALLOC_BY_TYPE(id->identity, pt, 1, 1, f);
			if (!var_pf) {
				scf_loge("var alloc error\n");
				return SCF_DFA_ERROR;
			}

			var_pf->const_flag = 1;
			var_pf->const_literal_flag = 1;

		} else {
			ret = scf_ast_find_variable(&var_pf, parse->ast, id->identity->text->data);
			if (ret < 0)
				return SCF_DFA_ERROR;

			if (!var_pf) {
				scf_loge("funcptr var '%s' not found\n", id->identity->text->data);
				return SCF_DFA_ERROR;
			}

			if (SCF_FUNCTION_PTR != var_pf->type || !var_pf->func_ptr) {
				scf_loge("invalid function ptr\n");
				return SCF_DFA_ERROR;
			}
		}

		node_pf = scf_node_alloc(NULL, var_pf->type, var_pf);
		if (!node_pf) {
			scf_loge("node alloc failed\n");
			return SCF_DFA_ERROR;
		}

		scf_stack_pop(d->current_identities);
		free(id);
		id = NULL;
	} else {
		// f()(), function f should return a function pointer
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	node_call = scf_node_alloc(w1, SCF_OP_CALL, NULL);
	if (!node_call) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}
	node_call->op = op;

	cd = calloc(1, sizeof(dfa_call_data_t));
	if (!cd) {
		scf_loge("dfa data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_logd("d->expr: %p\n", d->expr);

	cd->func           = node_pf;
	cd->call           = node_call;
	cd->parent_expr    = d->expr;
	d->expr            = NULL;
	d->expr_local_flag++;

	scf_stack_push(s, cd);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _call_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (words->size < 2) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];
	scf_stack_t*       s     = d->module_datas[dfa_module_call.index];

	dfa_call_data_t*   cd    = scf_stack_top(s);
	if (!cd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	cd->nb_rps++;

	scf_logd("cd->nb_lps: %d, cd->nb_rps: %d\n", cd->nb_lps, cd->nb_rps);

	if (cd->nb_rps < cd->nb_lps) {

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_rp"),      SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_comma"),   SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_lp_stat"), SCF_DFA_HOOK_POST);

		scf_logd("d->expr: %p\n", d->expr);
		return SCF_DFA_NEXT_WORD;
	}
	assert(cd->nb_rps == cd->nb_lps);

	scf_stack_pop(s);

	if (cd->parent_expr) {
		scf_expr_add_node(cd->parent_expr, cd->func);
		scf_expr_add_node(cd->parent_expr, cd->call);
	} else {
		scf_node_add_child(cd->call, cd->func);
	}

	if (cd->argv) {
		int i;
		for (i = 0; i < cd->argv->size; i++)
			scf_node_add_child(cd->call, cd->argv->data[i]);

		scf_vector_free(cd->argv);
		cd->argv = NULL;
	}

	// the last arg
	if (d->expr) {
		scf_node_add_child(cd->call, d->expr);
		d->expr = NULL;
	}

	if (cd->parent_expr)
		d->expr = cd->parent_expr;
	else
		d->expr = cd->call;

	d->expr_local_flag--;

	scf_logd("d->expr: %p\n", d->expr);

	free(cd);
	cd = NULL;

	return SCF_DFA_NEXT_WORD;
}

static int _call_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (words->size < 2) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_call.index];

	dfa_call_data_t*  cd    = scf_stack_top(s);
	if (!cd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (!cd->argv)
		cd->argv = scf_vector_alloc();

	scf_vector_add(cd->argv, d->expr);
	d->expr = NULL;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "call_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _dfa_init_module_call(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, call, lp,       scf_dfa_is_lp,    _call_action_lp);
	SCF_DFA_MODULE_NODE(dfa, call, rp,       scf_dfa_is_rp,    _call_action_rp);

	SCF_DFA_MODULE_NODE(dfa, call, lp_stat,  scf_dfa_is_lp,    _call_action_lp_stat);

	SCF_DFA_MODULE_NODE(dfa, call, comma,    scf_dfa_is_comma, _call_action_comma);

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_call.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_call.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_call(scf_dfa_t* dfa)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_call.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_call.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_call(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, call, lp,              lp);
	SCF_DFA_GET_MODULE_NODE(dfa, call, rp,              rp);
	SCF_DFA_GET_MODULE_NODE(dfa, call, comma,           comma);

	SCF_DFA_GET_MODULE_NODE(dfa, expr, entry,           expr);

	SCF_DFA_GET_MODULE_NODE(dfa, create,    create,   create);
	SCF_DFA_GET_MODULE_NODE(dfa, create,    identity, create_id);
	SCF_DFA_GET_MODULE_NODE(dfa, create,    rp,       create_rp);

	// no args
	scf_dfa_node_add_child(lp,       rp);

	// have args

	// arg: create class object, such as: list.push(new A);
	scf_dfa_node_add_child(lp,        create);
	scf_dfa_node_add_child(create_id, comma);
	scf_dfa_node_add_child(create_id, rp);
	scf_dfa_node_add_child(create_rp, comma);
	scf_dfa_node_add_child(create_rp, rp);
	scf_dfa_node_add_child(comma,     create);

	scf_dfa_node_add_child(lp,       expr);
	scf_dfa_node_add_child(expr,     comma);
	scf_dfa_node_add_child(comma,    expr);
	scf_dfa_node_add_child(expr,     rp);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_call =
{
	.name        = "call",
	.init_module = _dfa_init_module_call,
	.init_syntax = _dfa_init_syntax_call,

	.fini_module = _dfa_fini_module_call,
};

