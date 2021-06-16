#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_init_data;

int scf_array_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* var, scf_vector_t* init_exprs);
int scf_struct_init(scf_ast_t* ast, scf_lex_word_t* w, scf_variable_t* var, scf_vector_t* init_exprs);

int _expr_add_var(scf_parse_t* parse, dfa_parse_data_t* d);

typedef struct {
	int              nb_lbs;
	int              nb_rbs;

} data_module_data_t;

static int _do_data_init(scf_dfa_t* dfa, scf_vector_t* words, dfa_parse_data_t* d)
{
	scf_parse_t*        parse = dfa->priv;
	scf_variable_t*     var   = d->current_var;
	scf_lex_word_t*     w     = words->data[words->size - 1];
	data_module_data_t* md    = d->module_datas[dfa_module_init_data.index];
	dfa_init_expr_t*    ie;

	int ret = -1;
	int i   = 0;

	if (d->current_var->nb_dimentions > 0)
		ret = scf_array_init(parse->ast, w, d->current_var, d->init_exprs);

	else if (d->current_var->type >=  SCF_STRUCT)
		ret = scf_struct_init(parse->ast, w, d->current_var, d->init_exprs);

	if (ret < 0)
		goto error;

	if (d->current_var->global_flag) {

		for (i = 0; i < d->init_exprs->size; i++) {
			ie =        d->init_exprs->data[i];

			assert(!ie->current_index);

			ret = scf_expr_calculate(parse->ast, ie->expr, NULL);
			if (ret < 0)
				goto error;

			scf_expr_free(ie->expr);
			free(ie);
			ie = NULL;
		}
	} else {
		for (i = 0; i < d->init_exprs->size; i++) {
			ie =        d->init_exprs->data[i];

			assert(!ie->current_index);

			ret = scf_node_add_child((scf_node_t*)parse->ast->current_block, ie->expr);
			if (ret < 0)
				goto error;

			free(ie);
			ie = NULL;
		}
	}

error:
	for (; i < d->init_exprs->size; i++) {
		ie =        d->init_exprs->data[i];

		assert(!ie->current_index);

		scf_expr_free(ie->expr);
		free(ie);
		ie = NULL;
	}

	scf_vector_free(d->init_exprs);
	d->init_exprs = NULL;

	scf_vector_free(d->current_index);
	d->current_index = NULL;

	d->current_dim = -1;
	md->nb_lbs     = 0;
	md->nb_rbs     = 0;

	return ret;
}

static int _add_data_init_expr(scf_dfa_t* dfa, scf_vector_t* words, dfa_parse_data_t* d)
{
	assert(!d->expr->parent);
	assert(d->current_dim >= 0);

	scf_logi("current_dim: %d, d->expr: %p\n", d->current_dim, d->expr);

	dfa_init_expr_t* init_expr = malloc(sizeof(dfa_init_expr_t));
	if (!init_expr) {
		scf_loge("alloc init_expr failed\n");
		return SCF_DFA_ERROR;
	}

	init_expr->expr          = d->expr;
	d->expr                  = NULL;
	init_expr->current_index = scf_vector_clone(d->current_index);

	scf_vector_add(d->init_exprs, init_expr);
	return SCF_DFA_OK;
}

static int _data_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	data_module_data_t* md    = d->module_datas[dfa_module_init_data.index];

	if (d->current_dim < 0) {
		scf_logi("d->current_dim: %d\n", d->current_dim);
		return SCF_DFA_NEXT_SYNTAX;
	}

	if (d->expr) {
		if (_add_data_init_expr(dfa, words, d) < 0)
			return SCF_DFA_ERROR;
	}

	intptr_t index = (intptr_t)d->current_index->data[d->current_dim] + 1;
	d->current_index->data[d->current_dim] = (void*)index;

	scf_logi("\033[31m d->current_dim[%d]: %ld\033[0m\n", d->current_dim, index);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "init_data_comma"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}


static int _data_action_lb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (words->size < 2) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	scf_lex_word_t*     w     = words->data[words->size - 2];
	data_module_data_t* md    = d->module_datas[dfa_module_init_data.index]; 

	if (SCF_LEX_WORD_ASSIGN == w->type) {
		memset(md, 0, sizeof(data_module_data_t));

		assert(!d->init_exprs);
		d->init_exprs = scf_vector_alloc();

		d->current_dim       = -1;
		md->nb_lbs           = 0;
		md->nb_rbs           = 0;
		d->current_index     = scf_vector_alloc();

		d->expr_local_flag   = 1;

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "init_data_comma"), SCF_DFA_HOOK_POST);
	}

	d->expr = NULL;
	d->current_dim++;
	md->nb_lbs++;

	if (d->current_dim >= d->current_index->size)
		scf_vector_add(d->current_index, 0);
	assert(d->current_dim < d->current_index->size);

	int i;
	for (i = d->current_dim + 1; i < d->current_index->size; i++)
		d->current_index->data[i] = 0;

	return SCF_DFA_NEXT_WORD;
}

static int _data_action_rb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = data;
	data_module_data_t* md    = d->module_datas[dfa_module_init_data.index]; 

	if (d->expr) {
		dfa_identity_t* id = scf_stack_top(d->current_identities);

		if (id && id->identity) {
			if (_expr_add_var(parse, d) < 0)
				return SCF_DFA_ERROR;
		}

		if (_add_data_init_expr(dfa, words, d) < 0)
			return SCF_DFA_ERROR;
	}

	md->nb_rbs++;
	d->current_dim--;

	int i;
	for (i = d->current_dim + 1; i < d->current_index->size; i++)
		d->current_index->data[i] = 0;

	if (md->nb_rbs == md->nb_lbs) {
		d->expr_local_flag = 0;

		if (_do_data_init(dfa, words, d) < 0) {
			scf_loge("do data init failed\n");
			return SCF_DFA_ERROR;
		}

		scf_dfa_del_hook_by_name(&(dfa->hooks[SCF_DFA_HOOK_POST]), "init_data_comma");

		scf_logi("\n");
	}

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_init_data(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, init_data, comma, scf_dfa_is_comma, _data_action_comma);

	SCF_DFA_MODULE_NODE(dfa, init_data, lb,    scf_dfa_is_lb,    _data_action_lb);
	SCF_DFA_MODULE_NODE(dfa, init_data, rb,    scf_dfa_is_rb,    _data_action_rb);

	scf_parse_t*        parse = dfa->priv;
	dfa_parse_data_t*   d     = parse->dfa_data;
	data_module_data_t* md    = d->module_datas[dfa_module_init_data.index];

	assert(!md);

	md = calloc(1, sizeof(data_module_data_t));
	if (!md) {
		scf_loge("module data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_init_data.index] = md;

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_init_data(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, init_data, comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa, init_data, lb,        lb);
	SCF_DFA_GET_MODULE_NODE(dfa, init_data, rb,        rb);

	SCF_DFA_GET_MODULE_NODE(dfa, expr, entry,     expr);

	// empty init, use 0 to fill the data
	scf_dfa_node_add_child(lb,        rb);

	// multi-dimention data init
	scf_dfa_node_add_child(lb,        lb);
	scf_dfa_node_add_child(rb,        rb);
	scf_dfa_node_add_child(rb,        comma);
	scf_dfa_node_add_child(comma,     lb);

	// init expr for member of data
	scf_dfa_node_add_child(lb,        expr);
	scf_dfa_node_add_child(expr,      comma);
	scf_dfa_node_add_child(comma,     expr);
	scf_dfa_node_add_child(expr,      rb);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_init_data =
{
	.name        = "init_data",
	.init_module = _dfa_init_module_init_data,
	.init_syntax = _dfa_init_syntax_init_data,
};

