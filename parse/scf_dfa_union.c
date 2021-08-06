#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_union;

typedef struct {
	scf_lex_word_t*  current_identity;

	scf_block_t*     parent_block;

	scf_type_t*      current_union;

	scf_dfa_hook_t*  hook;

	int              nb_lbs;
	int              nb_rbs;

} union_module_data_t;

static int _union_is_union(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;
	return SCF_LEX_WORD_KEY_UNION == w->type;
}

static int _union_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];
	scf_lex_word_t*       w     = words->data[words->size - 1];

	if (md->current_identity) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_type_t* t = scf_block_find_type(parse->ast->current_block, w->text->data);
	if (!t) {
		t = scf_type_alloc(w, w->text->data, SCF_STRUCT + parse->ast->nb_structs, 0);
		if (!t) {
			scf_loge("type alloc failed\n");
			return SCF_DFA_ERROR;
		}

		parse->ast->nb_structs++;
		t->node.union_flag = 1;
		scf_scope_push_type(parse->ast->current_block->scope, t);
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)t);
	}

	md->current_identity = w;
	md->parent_block     = parse->ast->current_block;

	return SCF_DFA_NEXT_WORD;
}

static int _union_action_lb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];
	scf_lex_word_t*       w     = words->data[words->size - 1];
	scf_type_t*           t     = NULL;

	if (md->current_identity) {

		t = scf_block_find_type(parse->ast->current_block, md->current_identity->text->data);
		if (!t) {
			scf_loge("type '%s' not found\n", md->current_identity->text->data);
			return SCF_DFA_ERROR;
		}
	} else {
		t = scf_type_alloc(w, "anonymous", SCF_STRUCT + parse->ast->nb_structs, 0);
		if (!t) {
			scf_loge("type alloc failed\n");
			return SCF_DFA_ERROR;
		}

		parse->ast->nb_structs++;
		t->node.union_flag = 1;
		scf_scope_push_type(parse->ast->current_block->scope, t);
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)t);
	}

	if (!t->scope)
		t->scope = scf_scope_alloc(w, "union");

	md->parent_block  = parse->ast->current_block;
	md->current_union = t;
	md->nb_lbs++;

	parse->ast->current_block = (scf_block_t*)t;

	md->hook = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "union_semicolon"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _union_calculate_size(scf_dfa_t* dfa, scf_type_t* s)
{
	scf_parse_t* parse = dfa->priv;

	int max_size = 0;
	int i;
	int j;
	for (i = 0; i < s->scope->vars->size; i++) {

		scf_variable_t* v = s->scope->vars->data[i];
		assert(v->size >= 0);

		int size = 0;

		if (v->nb_dimentions > 0) {
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
			size = v->size * v->capacity;
		} else {
			size = v->size;
		}

		if (max_size < size)
			max_size = size;

		scf_logi("union '%s', member: '%s', size: %d, v->dim: %d, v->capacity: %d\n",
				s->name->data, v->w->text->data, v->size, v->nb_dimentions, v->capacity);
	}
	s->size = max_size;

	scf_logi("union '%s', size: %d\n", s->name->data, s->size);
	return 0;
}

static int _union_action_rb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];

	if (_union_calculate_size(dfa, md->current_union) < 0) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	md->nb_rbs++;

	assert(md->nb_rbs == md->nb_lbs);

	parse->ast->current_block = md->parent_block;
	md->parent_block = NULL;

	return SCF_DFA_NEXT_WORD;
}

static int _union_action_var(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];
	scf_lex_word_t*       w     = words->data[words->size - 1];

	if (!md->current_union) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_variable_t* var = scf_variable_alloc(w, md->current_union);
	if (!var) {
		scf_loge("var alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_scope_push_var(parse->ast->current_block->scope, var);

	scf_logi("union var: '%s', type: %d, size: %d\n", w->text->data, var->type, var->size);

	return SCF_DFA_NEXT_WORD;
}

static int _union_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];

	if (md->nb_rbs == md->nb_lbs) {
		scf_logi("SCF_DFA_OK\n");

//		parse->ast->current_block = md->parent_block;
//		md->parent_block     = NULL;

		md->current_identity = NULL;
		md->current_union    = NULL;
		md->nb_lbs           = 0;
		md->nb_rbs           = 0;

		scf_dfa_del_hook(&(dfa->hooks[SCF_DFA_HOOK_POST]), md->hook);
		md->hook             = NULL;

		return SCF_DFA_OK;
	}

	md->hook = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "union_semicolon"), SCF_DFA_HOOK_POST);

	scf_logi("\n");
	return SCF_DFA_SWITCH_TO;
}

static int _dfa_init_module_union(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, union, _union,    _union_is_union,      NULL);

	SCF_DFA_MODULE_NODE(dfa, union, identity,  scf_dfa_is_identity,  _union_action_identity);

	SCF_DFA_MODULE_NODE(dfa, union, lb,        scf_dfa_is_lb,        _union_action_lb);
	SCF_DFA_MODULE_NODE(dfa, union, rb,        scf_dfa_is_rb,        _union_action_rb);
	SCF_DFA_MODULE_NODE(dfa, union, semicolon, scf_dfa_is_semicolon, _union_action_semicolon);

	SCF_DFA_MODULE_NODE(dfa, union, var,       scf_dfa_is_identity,  _union_action_var);

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = parse->dfa_data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];

	assert(!md);

	md = calloc(1, sizeof(union_module_data_t));
	if (!md) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_union.index] = md;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_union(scf_dfa_t* dfa)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = parse->dfa_data;
	union_module_data_t*  md    = d->module_datas[dfa_module_union.index];

	if (md) {
		free(md);
		md = NULL;
		d->module_datas[dfa_module_union.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_union(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, union,  _union,    _union);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  lb,        lb);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  rb,        rb);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  var,       var);

	SCF_DFA_GET_MODULE_NODE(dfa, type, entry,    member);

	scf_vector_add(dfa->syntaxes,     _union);

	// union start
	scf_dfa_node_add_child(_union,    identity);

	scf_dfa_node_add_child(identity,  semicolon);
	scf_dfa_node_add_child(identity,  lb);

	// anonymous union, will be only used in struct or class to define vars shared the same memory
	scf_dfa_node_add_child(_union,    lb);

	// empty union
	scf_dfa_node_add_child(lb,        rb);

	// member var
	scf_dfa_node_add_child(lb,        member);
	scf_dfa_node_add_child(member,    semicolon);
	scf_dfa_node_add_child(semicolon, rb);
	scf_dfa_node_add_child(semicolon, member);

	// end
	scf_dfa_node_add_child(rb,        var);
	scf_dfa_node_add_child(var,       semicolon);
	scf_dfa_node_add_child(rb,        semicolon);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_union = 
{
	.name        = "union",
	.init_module = _dfa_init_module_union,
	.init_syntax = _dfa_init_syntax_union,

	.fini_module = _dfa_fini_module_union,
};

