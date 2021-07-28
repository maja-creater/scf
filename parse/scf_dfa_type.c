#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_type;

static int _type_is__struct(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_CLASS  == w->type
		|| SCF_LEX_WORD_KEY_STRUCT == w->type;
}

static int _type_action_const(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->const_flag = 1;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_extern(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->extern_flag = 1;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_static(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->static_flag = 1;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_inline(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->inline_flag = 1;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_base_type(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->current_identities;

	dfa_identity_t*   id    = calloc(1, sizeof(dfa_identity_t));
	if (!id)
		return SCF_DFA_ERROR;

	id->type = scf_block_find_type(parse->ast->current_block, w->text->data);
	if (!id->type) {
		scf_loge("can't find type '%s'\n", w->text->data);

		free(id);
		return SCF_DFA_ERROR;
	}

	if (scf_stack_push(s, id) < 0) {
		free(id);
		return SCF_DFA_ERROR;
	}

	id->type_w      = w;

	id->const_flag  = d->const_flag;
	id->static_flag = d->static_flag;
	id->extern_flag = d->extern_flag;
	id->inline_flag = d->inline_flag;

	d ->const_flag  = 0;
	d ->static_flag = 0;
	d ->extern_flag = 0;
	d ->inline_flag = 0;

	return SCF_DFA_NEXT_WORD;
}

static scf_function_t* _type_find_function(scf_block_t* b, const char* name)
{
	while (b) {
		if (!b->node.file_flag && !b->node.root_flag) {
			b = (scf_block_t*)b->node.parent;
			continue;
		}

		assert(b->scope);

		scf_function_t* f = scf_scope_find_function(b->scope, name);
		if (f)
			return f;

		b = (scf_block_t*)b->node.parent;
	}

	return NULL;
}

int _type_find_type(scf_dfa_t* dfa, dfa_identity_t* id)
{
	scf_parse_t* parse = dfa->priv;

	if (!id->identity)
		return 0;

	id->type = scf_block_find_type(parse->ast->current_block, id->identity->text->data);
	if (!id->type) {

		int ret = scf_ast_find_global_type(&id->type, parse->ast, id->identity->text->data);
		if (ret < 0) {
			scf_loge("find global function error\n");
			return SCF_DFA_ERROR;
		}

		if (!id->type) {
			id->type = scf_block_find_type_type(parse->ast->current_block, SCF_FUNCTION_PTR);

			if (!id->type) {
				scf_loge("function ptr not support\n");
				return SCF_DFA_ERROR;
			}
		}

		if (SCF_FUNCTION_PTR == id->type->type) {

			id->func_ptr = _type_find_function(parse->ast->current_block, id->identity->text->data);

			if (!id->func_ptr) {
				scf_loge("can't find funcptr type '%s'\n", id->identity->text->data);
				return SCF_DFA_ERROR;
			}
		}
	}

	id->type_w   = id->identity;
	id->identity = NULL;
	return 0;
}

static int _type_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->current_identities;
	dfa_identity_t*   id    = NULL;

	if (s->size > 0) {
		id = scf_stack_top(s);

		int ret = _type_find_type(dfa, id);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	id  = calloc(1, sizeof(dfa_identity_t));
	if (!id)
		return SCF_DFA_ERROR;

	if (scf_stack_push(s, id) < 0) {
		free(id);
		return SCF_DFA_ERROR;
	}
	id->identity = w;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_star(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d  = data;
	dfa_identity_t*   id = scf_stack_top(d->current_identities);

	assert(id);

	if (!id->type) {
		int ret = _type_find_type(dfa, id);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	id->nb_pointers++;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d  = data;
	dfa_identity_t*   id = scf_stack_top(d->current_identities);

	assert(id);

	if (!id->type) {
		int ret = _type_find_type(dfa, id);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_type(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_ENTRY(dfa, type);

	SCF_DFA_MODULE_NODE(dfa, type, _struct,   _type_is__struct,     NULL);

	SCF_DFA_MODULE_NODE(dfa, type, _const,    scf_dfa_is_const,     _type_action_const);
	SCF_DFA_MODULE_NODE(dfa, type, _static,   scf_dfa_is_static,    _type_action_static);
	SCF_DFA_MODULE_NODE(dfa, type, _extern,   scf_dfa_is_extern,    _type_action_extern);
	SCF_DFA_MODULE_NODE(dfa, type, _inline,   scf_dfa_is_inline,    _type_action_inline);

	SCF_DFA_MODULE_NODE(dfa, type, base_type, scf_dfa_is_base_type, _type_action_base_type);
	SCF_DFA_MODULE_NODE(dfa, type, identity,  scf_dfa_is_identity,  _type_action_identity);
	SCF_DFA_MODULE_NODE(dfa, type, star,      scf_dfa_is_star,      _type_action_star);
	SCF_DFA_MODULE_NODE(dfa, type, comma,     scf_dfa_is_comma,     _type_action_comma);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_type(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, type,     entry,     entry);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     _const,    _const);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     _static,   _static);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     _extern,   _extern);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     _inline,   _inline);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     _struct,   _struct);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     identity,  var_name);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  type_name);


	scf_vector_add(dfa->syntaxes,     entry);

	scf_dfa_node_add_child(entry,     _static);
	scf_dfa_node_add_child(entry,     _extern);
	scf_dfa_node_add_child(entry,     _const);
	scf_dfa_node_add_child(entry,     _inline);

	scf_dfa_node_add_child(entry,     _struct);
	scf_dfa_node_add_child(entry,     base_type);
	scf_dfa_node_add_child(entry,     type_name);

	scf_dfa_node_add_child(_static,   _struct);
	scf_dfa_node_add_child(_static,   base_type);
	scf_dfa_node_add_child(_static,   type_name);

	scf_dfa_node_add_child(_extern,   _struct);
	scf_dfa_node_add_child(_extern,   base_type);
	scf_dfa_node_add_child(_extern,   type_name);

	scf_dfa_node_add_child(_const,    _struct);
	scf_dfa_node_add_child(_const,    base_type);
	scf_dfa_node_add_child(_const,    type_name);

	scf_dfa_node_add_child(_inline,   _struct);
	scf_dfa_node_add_child(_inline,   base_type);
	scf_dfa_node_add_child(_inline,   type_name);

	scf_dfa_node_add_child(_static,   _inline);
	scf_dfa_node_add_child(_static,   _const);
	scf_dfa_node_add_child(_extern,   _const);
	scf_dfa_node_add_child(_inline,   _const);

	scf_dfa_node_add_child(_struct,   type_name);

	// multi-pointer
	scf_dfa_node_add_child(star,      star);
	scf_dfa_node_add_child(star,      var_name);

	scf_dfa_node_add_child(base_type, star);
	scf_dfa_node_add_child(type_name, star);

	scf_dfa_node_add_child(base_type, var_name);
	scf_dfa_node_add_child(type_name, var_name);

	// multi-return-value function
	scf_dfa_node_add_child(base_type, comma);
	scf_dfa_node_add_child(star,      comma);
	scf_dfa_node_add_child(comma,     _struct);
	scf_dfa_node_add_child(comma,     base_type);
	scf_dfa_node_add_child(comma,     type_name);

	int i;
	for (i = 0; i < base_type->childs->size; i++) {
		scf_dfa_node_t* n = base_type->childs->data[i];

		scf_logi("n->name: %s\n", n->name);
	}

	scf_logi("\n");
	return SCF_DFA_OK;
}

scf_dfa_module_t dfa_module_type = 
{
	.name        = "type",
	.init_module = _dfa_init_module_type,
	.init_syntax = _dfa_init_syntax_type,
};

