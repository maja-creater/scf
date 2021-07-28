#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_container;

typedef struct {

	int              nb_lps;
	int              nb_rps;

	scf_node_t*      container;

	scf_expr_t*      parent_expr;

} dfa_container_data_t;

static int _container_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t*     d  = data;
	scf_stack_t*          s  = d->module_datas[dfa_module_container.index];
	dfa_container_data_t* cd = scf_stack_top(s);

	if (!cd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	cd->nb_lps++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _container_action_container(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	scf_stack_t*          s     = d->module_datas[dfa_module_container.index];

	dfa_container_data_t* cd    = calloc(1, sizeof(dfa_container_data_t));
	if (!cd) {
		scf_loge("module data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_node_t* container = scf_node_alloc(w, SCF_OP_CONTAINER, NULL);
	if (!container) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_logd("d->expr: %p\n", d->expr);

	cd->container   = container;
	cd->parent_expr = d->expr;
	d->expr         = NULL;
	d->expr_local_flag++;
	d->nb_containers++;

	scf_stack_push(s, cd);

	return SCF_DFA_NEXT_WORD;
}

static int _container_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _container_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	scf_stack_t*          s     = d->module_datas[dfa_module_container.index];
	dfa_container_data_t* cd    = scf_stack_top(s);

	if (0 == cd->container->nb_nodes) {
		if (!d->expr) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		scf_node_add_child(cd->container, d->expr);
		d->expr = NULL;
		d->expr_local_flag--;

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_comma"),   SCF_DFA_HOOK_PRE);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_comma"),   SCF_DFA_HOOK_POST);

	} else if (1 == cd->container->nb_nodes) {

		scf_variable_t* v;
		dfa_identity_t* id;
		scf_node_t*     node;

		id = scf_stack_pop(d->current_identities);
		assert(id);

		if (!id->type) {
			if (scf_ast_find_type(&id->type, parse->ast, id->identity->text->data) < 0) {
				free(id);
				return SCF_DFA_ERROR;
			}

			if (!id->type) {
				scf_loge("can't find type '%s'\n", w->text->data);
				free(id);
				return SCF_DFA_ERROR;
			}

			if (id->type->type < SCF_STRUCT) {
				scf_loge("'%s' is not a class or struct\n", w->text->data);
				free(id);
				return SCF_DFA_ERROR;
			}

			id->type_w   = id->identity;
			id->identity = NULL;
		}

		v = SCF_VAR_ALLOC_BY_TYPE(id->type_w, id->type, 0, 1, NULL);
		if (!v) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		node = scf_node_alloc(NULL, v->type, v);
		if (!node) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}

		scf_node_add_child(cd->container, node);

		scf_loge("\n");
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_rp"),   SCF_DFA_HOOK_PRE);

		free(id);
		id = NULL;
	} else {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_loge("\n");
	return SCF_DFA_SWITCH_TO;
}

static int _container_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	scf_lex_word_t*       w     = words->data[words->size - 1];
	scf_stack_t*          s     = d->module_datas[dfa_module_container.index];
	dfa_container_data_t* cd    = scf_stack_top(s);

	if (d->current_va_arg)
		return SCF_DFA_NEXT_SYNTAX;

	if (!cd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (cd->container->nb_nodes >= 3) {
		scf_stack_pop(s);
		free(cd);
		cd = NULL;
		return SCF_DFA_NEXT_WORD;
	}

	cd->nb_rps++;

	scf_logd("cd->nb_lps: %d, cd->nb_rps: %d\n", cd->nb_lps, cd->nb_rps);

	if (cd->nb_rps < cd->nb_lps) {

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_rp"),      SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_comma"),   SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "container_lp_stat"), SCF_DFA_HOOK_POST);

		return SCF_DFA_NEXT_WORD;
	}
	assert(cd->nb_rps == cd->nb_lps);

	scf_variable_t* v;
	dfa_identity_t* id;
	scf_node_t*     node;
	scf_type_t*     t;

	id = scf_stack_pop(d->current_identities);
	assert(id && id->identity);

	t = NULL;
	if (scf_ast_find_type_type(&t, parse->ast, cd->container->nodes[1]->type) < 0)
		return SCF_DFA_ERROR;
	assert(t);

	v = scf_scope_find_variable(t->scope, id->identity->text->data);
	assert(v);

	node = scf_node_alloc(NULL, v->type, v);
	if (!node) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_node_add_child(cd->container, node);

	scf_logi("cd->container->nb_nodes: %d\n", cd->container->nb_nodes);

	if (cd->parent_expr) {
		if (scf_expr_add_node(cd->parent_expr, cd->container) < 0) {
			scf_loge("\n");
			return SCF_DFA_ERROR;
		}
		d->expr = cd->parent_expr;
	} else
		d->expr = cd->container;

	d->nb_containers--;

	scf_logi("d->expr: %p, d->expr_local_flag: %d, d->nb_containers: %d\n", d->expr, d->expr_local_flag, d->nb_containers);

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_container(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, container, container, scf_dfa_is_container, _container_action_container);
	SCF_DFA_MODULE_NODE(dfa, container, lp,        scf_dfa_is_lp,        _container_action_lp);
	SCF_DFA_MODULE_NODE(dfa, container, rp,        scf_dfa_is_rp,        _container_action_rp);
	SCF_DFA_MODULE_NODE(dfa, container, lp_stat,   scf_dfa_is_lp,        _container_action_lp_stat);
	SCF_DFA_MODULE_NODE(dfa, container, comma,     scf_dfa_is_comma,     _container_action_comma);

	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_container.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_container.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_container(scf_dfa_t* dfa)
{
	scf_parse_t*       parse = dfa->priv;
	dfa_parse_data_t*  d     = parse->dfa_data;
	scf_stack_t*       s     = d->module_datas[dfa_module_container.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_container.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_container(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa,      container,   container, container);
	SCF_DFA_GET_MODULE_NODE(dfa,      container,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa,      container,   rp,        rp);
	SCF_DFA_GET_MODULE_NODE(dfa,      container,   comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa,      expr,        entry,     expr);

	SCF_DFA_GET_MODULE_NODE(dfa,      type,        entry,     type);
	SCF_DFA_GET_MODULE_NODE(dfa,      type,        base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa,      identity,    identity,  identity);

	scf_dfa_node_add_child(container, lp);
	scf_dfa_node_add_child(lp,        expr);
	scf_dfa_node_add_child(expr,      comma);

	scf_dfa_node_add_child(comma,     type);
	scf_dfa_node_add_child(base_type, comma);
	scf_dfa_node_add_child(identity,  comma);

	scf_dfa_node_add_child(comma,     identity);
	scf_dfa_node_add_child(identity,  rp);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_container =
{
	.name        = "container",
	.init_module = _dfa_init_module_container,
	.init_syntax = _dfa_init_syntax_container,

	.fini_module = _dfa_fini_module_container,
};

