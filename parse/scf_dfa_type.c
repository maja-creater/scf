#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_type;

static int _type_is__struct(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_STRUCT == w->type;
}

static int _type_action_base_type(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_parse_data_t* d     = data;

	d->current_type = scf_block_find_type(parse->ast->current_block, w->text->data);
	if (!d->current_type) {
		scf_loge("can't find type '%s'\n", w->text->data);
		return SCF_DFA_ERROR;
	}

	d->current_type_w   = w;
	d->current_identity = NULL;

	return SCF_DFA_NEXT_WORD;
}

static scf_function_t* _type_find_function(scf_block_t* b, const char* name)
{
	while (b) {
		if (!b->file_flag && !b->root_flag) {
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

static int _type_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_parse_data_t* d     = data;

	if (d->current_identity) {

		d->current_type = scf_block_find_type(parse->ast->current_block, d->current_identity->text->data);

		if (!d->current_type) {

			d->current_type = scf_block_find_type_type(parse->ast->current_block, SCF_FUNCTION_PTR);

			if (!d->current_type) {
				scf_loge("function ptr not support\n");
				return SCF_DFA_ERROR;
			}
		}

		if (SCF_FUNCTION_PTR == d->current_type->type) {

			d->func_ptr = _type_find_function(parse->ast->current_block, d->current_identity->text->data);

			if (!d->func_ptr) {
				scf_loge("can't find funcptr type '%s'\n", d->current_identity->text->data);
				return SCF_DFA_ERROR;
			}
		}

		d->current_type_w = w;
	}

	d->current_identity = w;

	return SCF_DFA_NEXT_WORD;
}

static int _type_action_star(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d = data;

	d->nb_pointers++;

	return SCF_DFA_NEXT_WORD;
}

static int _dfa_init_module_type(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_ENTRY(dfa, type);

	SCF_DFA_MODULE_NODE(dfa, type, _struct,   _type_is__struct,     NULL);
	SCF_DFA_MODULE_NODE(dfa, type, base_type, scf_dfa_is_base_type, _type_action_base_type);
	SCF_DFA_MODULE_NODE(dfa, type, identity,  scf_dfa_is_identity,  _type_action_identity);
	SCF_DFA_MODULE_NODE(dfa, type, star,      scf_dfa_is_star,      _type_action_star);

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_type(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, type,     entry,     entry);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     _struct,   _struct);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     identity,  var_name);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,      star);

	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  type_name);


	scf_vector_add(dfa->syntaxes,     entry);

	scf_dfa_node_add_child(entry,     _struct);
	scf_dfa_node_add_child(entry,     base_type);
	scf_dfa_node_add_child(entry,     type_name);

	scf_dfa_node_add_child(_struct,   type_name);

	// multi-pointer
	scf_dfa_node_add_child(star,      star);
	scf_dfa_node_add_child(star,      var_name);

	scf_dfa_node_add_child(base_type, star);
	scf_dfa_node_add_child(type_name, star);

	scf_dfa_node_add_child(base_type, var_name);
	scf_dfa_node_add_child(type_name, var_name);

	scf_logi("\n");
	return SCF_DFA_OK;
}

scf_dfa_module_t dfa_module_type = 
{
	.name        = "type",
	.init_module = _dfa_init_module_type,
	.init_syntax = _dfa_init_syntax_type,
};

