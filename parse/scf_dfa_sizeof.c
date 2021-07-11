#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_sizeof;

typedef struct {

	int              nb_lps;
	int              nb_rps;

	scf_node_t*      _sizeof;

	scf_expr_t*      parent_expr;

} dfa_sizeof_data_t;

static int _sizeof_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t*  d     = data;
	scf_stack_t*       s     = d->module_datas[dfa_module_sizeof.index];
	dfa_sizeof_data_t* sd    = scf_stack_top(s);

	if (!sd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	sd->nb_lps++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "sizeof_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _sizeof_action_sizeof(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];
	scf_stack_t*       s     = d->module_datas[dfa_module_sizeof.index];

	dfa_sizeof_data_t* sd    = calloc(1, sizeof(dfa_sizeof_data_t));
	if (!sd) {
		scf_loge("module data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_node_t* _sizeof = scf_node_alloc(w, SCF_OP_SIZEOF, NULL);
	if (!_sizeof) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_logd("d->expr: %p\n", d->expr);

	sd->_sizeof     = _sizeof;
	sd->parent_expr = d->expr;
	d->expr         = NULL;
	d->expr_local_flag++;
	d->nb_sizeofs++;

	scf_stack_push(s, sd);

	return SCF_DFA_NEXT_WORD;
}

static int _sizeof_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "sizeof_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "sizeof_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _sizeof_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = data;
	scf_lex_word_t*    w     = words->data[words->size - 1];
	scf_stack_t*       s     = d->module_datas[dfa_module_sizeof.index];
	dfa_sizeof_data_t* sd    = scf_stack_top(s);

	if (d->current_va_arg)
		return SCF_DFA_NEXT_SYNTAX;

	if (!sd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	sd->nb_rps++;

	scf_logd("sd->nb_lps: %d, sd->nb_rps: %d\n", sd->nb_lps, sd->nb_rps);

	if (sd->nb_rps < sd->nb_lps) {

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "sizeof_rp"),      SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "sizeof_lp_stat"), SCF_DFA_HOOK_POST);

		return SCF_DFA_NEXT_WORD;
	}
	assert(sd->nb_rps == sd->nb_lps);

	if (d->expr) {
		scf_node_add_child(sd->_sizeof, d->expr);
		d->expr = NULL;

	} else if (d->current_identities->size > 0) {

		scf_variable_t* v;
		dfa_identity_t* id;
		scf_node_t*     n;
		scf_expr_t*     e;
		scf_type_t*     t;

		id = scf_stack_pop(d->current_identities);
		assert(id && id->type);

		scf_loge("\n");

		if (id->nb_pointers > 0) {

			t = scf_block_find_type_type(parse->ast->current_block, SCF_VAR_INTPTR);
			assert(t);

			v = SCF_VAR_ALLOC_BY_TYPE(sd->_sizeof->w, t, 1, 0, NULL);
			if (!v) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}
			v->data.i = t->size;

			n = scf_node_alloc(NULL, SCF_VAR_INTPTR, v);
			if (!n) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			scf_node_free(sd->_sizeof);
			sd->_sizeof = n;
		} else {
			v = SCF_VAR_ALLOC_BY_TYPE(sd->_sizeof->w, id->type, 1, 0, NULL);
			if (!v) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			n = scf_node_alloc(NULL, v->type, v);
			if (!n) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			e = scf_expr_alloc();
			if (!n) {
				scf_loge("\n");
				return SCF_DFA_ERROR;
			}

			scf_expr_add_node(e, n);
			scf_node_add_child(sd->_sizeof, e);
		}

		free(id);
		id = NULL;
	} else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_stack_pop(s);

	if (sd->parent_expr) {
		scf_expr_add_node(sd->parent_expr, sd->_sizeof);
		d->expr = sd->parent_expr;
	} else
		d->expr = sd->_sizeof;

	d->expr_local_flag--;
	d->nb_sizeofs--;

	scf_logi("d->expr: %p, d->nb_sizeofs: %d\n", d->expr, d->nb_sizeofs);

	free(sd);
	sd = NULL;

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_sizeof(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, sizeof, _sizeof,  scf_dfa_is_sizeof, _sizeof_action_sizeof);
	SCF_DFA_MODULE_NODE(dfa, sizeof, lp,       scf_dfa_is_lp,     _sizeof_action_lp);
	SCF_DFA_MODULE_NODE(dfa, sizeof, rp,       scf_dfa_is_rp,     _sizeof_action_rp);
	SCF_DFA_MODULE_NODE(dfa, sizeof, lp_stat,  scf_dfa_is_lp,     _sizeof_action_lp_stat);

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_sizeof.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_sizeof.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_sizeof(scf_dfa_t* dfa)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_sizeof.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_sizeof.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_sizeof(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, sizeof,   _sizeof,   _sizeof);
	SCF_DFA_GET_MODULE_NODE(dfa, sizeof,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, sizeof,   rp,        rp);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,     entry,     expr);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     entry,     type);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  identity);

	scf_dfa_node_add_child(_sizeof,   lp);
	scf_dfa_node_add_child(lp,        expr);
	scf_dfa_node_add_child(expr,      rp);

	scf_dfa_node_add_child(lp,        type);
	scf_dfa_node_add_child(base_type, rp);
	scf_dfa_node_add_child(identity,  rp);
	scf_dfa_node_add_child(star,      rp);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_sizeof =
{
	.name        = "sizeof",
	.init_module = _dfa_init_module_sizeof,
	.init_syntax = _dfa_init_syntax_sizeof,

	.fini_module = _dfa_fini_module_sizeof,
};

