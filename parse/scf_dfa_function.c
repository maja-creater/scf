#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_function;

typedef struct {

	scf_block_t*     parent_block;

	scf_dfa_hook_t*  hook_end;

} dfa_fun_data_t;

int _function_add_function(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	if (d->current_identities->size != 2) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	scf_parse_t*    parse = dfa->priv;
	dfa_identity_t* id1   = scf_stack_pop(d->current_identities);
	dfa_identity_t* id0   = scf_stack_pop(d->current_identities);

	if (!id0 || !id0->type) {
		scf_loge("function ret type not found\n");
		return SCF_DFA_ERROR;
	}

	if (!id1 || !id1->identity) {
		scf_loge("function identity not found\n");
		return SCF_DFA_ERROR;
	}

	scf_function_t* f = scf_function_alloc(id1->identity);
	if (!f) {
		scf_loge("function alloc failed\n");
		return SCF_DFA_ERROR;
	}

	scf_logi("function: %s,line:%d,pos:%d\n",
			f->node.w->text->data, f->node.w->line, f->node.w->pos);

	d->current_function = f;

	f->ret = SCF_VAR_ALLOC_BY_TYPE(id1->identity, id0->type, id0->const_flag, id0->nb_pointers, NULL);
	if (!f->ret) {
		scf_loge("function return value alloc failed\n");

		scf_function_free(f);
		f = NULL;
		return SCF_DFA_ERROR;
	}

	free(id0);
	free(id1);
	return SCF_DFA_NEXT_WORD;
}

int _function_add_arg(scf_dfa_t* dfa, dfa_parse_data_t* d)
{
	dfa_identity_t* t = NULL;
	dfa_identity_t* v = NULL;

	switch (d->current_identities->size) {
		case 0:
			break;
		case 1:
			t = scf_stack_pop(d->current_identities);
			assert(t && t->type);
			break;
		case 2:
			v = scf_stack_pop(d->current_identities);
			t = scf_stack_pop(d->current_identities);
			assert(t && t->type);
			assert(v && v->identity);
			break;
		default:
			scf_loge("\n");
			return SCF_DFA_ERROR;
			break;
	};

	if (t && t->type) {
		scf_variable_t* arg = NULL;
		scf_lex_word_t* w   = NULL;

		if (v && v->identity)
			w = v->identity;

		arg = SCF_VAR_ALLOC_BY_TYPE(w, t->type, t->const_flag, t->nb_pointers, t->func_ptr);
		if (!arg)
			return SCF_DFA_ERROR;

		scf_vector_add(d->current_function->argv, arg);
		scf_scope_push_var(d->current_function->scope, arg);
		arg->refs++;
		arg->arg_flag   = 1;
		arg->local_flag = 1;

		if (v)
			free(v);
		free(t);

		d->argc++;
	}

	return SCF_DFA_NEXT_WORD;
}

static int _function_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d   = data;

	if (_function_add_arg(dfa, d) < 0) {
		scf_loge("function add arg failed\n");
		return SCF_DFA_ERROR;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "function_comma"), SCF_DFA_HOOK_PRE);

	return SCF_DFA_NEXT_WORD;
}

static int _function_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_fun_data_t*   fd    = d->module_datas[dfa_module_function.index];

	assert(!d->current_node);

	if (_function_add_function(dfa, d) < 0)
		return SCF_DFA_ERROR;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "function_rp"), SCF_DFA_HOOK_PRE);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "function_comma"), SCF_DFA_HOOK_PRE);

	d->argc = 0;
	d->nb_lps++;

	return SCF_DFA_NEXT_WORD;
}

static int _function_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	dfa_fun_data_t*   fd    = d->module_datas[dfa_module_function.index];

	d->nb_rps++;

	if (d->nb_rps < d->nb_lps) {
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "function_rp"), SCF_DFA_HOOK_PRE);
		return SCF_DFA_NEXT_WORD;
	}

	if (_function_add_arg(dfa, d) < 0) {
		scf_loge("function add arg failed\n");
		return SCF_DFA_ERROR;
	}

	scf_function_t* f = NULL;

	if (parse->ast->current_block->node.type >= SCF_STRUCT) {

		scf_type_t* t = (scf_type_t*)parse->ast->current_block;

		if (!t->node.class_flag) {
			scf_loge("only class has member function\n");
			return SCF_DFA_ERROR;
		}

		assert(t->scope);

		f = scf_scope_find_same_function(t->scope, d->current_function);

	} else {
		scf_block_t* b = parse->ast->current_block;

		while (b) {
			if (!b->node.root_flag && !b->node.file_flag) {
				scf_loge("function should be defined in file or global, or class\n");
				return SCF_DFA_ERROR;
			}

			assert(b->scope);

			f = scf_scope_find_function(b->scope, d->current_function->node.w->text->data);
			if (f) {
				if (scf_function_same(f, d->current_function))
					break;

				scf_loge("repeated declare function '%s', first in line: %d, second in line: %d, function overloading only can do in class\n",
						f->node.w->text->data, f->node.w->line, d->current_function->node.w->line);
				return SCF_DFA_ERROR;
			}

			b = (scf_block_t*)b->node.parent;
		}
	}

	if (f) {
		if (!f->node.define_flag) {
			int i;

			for (i = 0; i < f->argv->size; i++) {
				scf_variable_t* v0 = f->argv->data[i];
				scf_variable_t* v1 = d->current_function->argv->data[i];

				if (v1->w) {
					if (v0->w)
						scf_lex_word_free(v0->w);
					v0->w = scf_lex_word_clone(v1->w);
				}
			}

			scf_function_free(d->current_function);
			d->current_function = f;

		} else {
			scf_lex_word_t* w = dfa->ops->pop_word(dfa);

			if (SCF_LEX_WORD_SEMICOLON != w->type) {

				scf_loge("repeated define function '%s', first in line: %d, second in line: %d\n",
						f->node.w->text->data, f->node.w->line, d->current_function->node.w->line); 

				dfa->ops->push_word(dfa, w);
				return SCF_DFA_ERROR;
			}

			dfa->ops->push_word(dfa, w);
		}
	} else {
		scf_scope_push_function(parse->ast->current_block->scope, d->current_function);
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)d->current_function);
	}

	fd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "function_end"), SCF_DFA_HOOK_END);
	assert(fd->hook_end);

	fd->parent_block = parse->ast->current_block;
	parse->ast->current_block = (scf_block_t*)d->current_function;

	return SCF_DFA_NEXT_WORD;
}

static int _function_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	dfa_fun_data_t*   fd    = d->module_datas[dfa_module_function.index];

	parse->ast->current_block  = (scf_block_t*)(fd->parent_block);

	if (d->current_function->node.nb_nodes > 0)
		d->current_function->node.define_flag = 1;

	fd->parent_block = NULL;
	fd->hook_end     = NULL;

	d->current_function = NULL;
	d->argc   = 0;
	d->nb_lps = 0;
	d->nb_rps = 0;

	scf_logi("\n");
	return SCF_DFA_OK;
}

static int _dfa_init_module_function(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, function, comma,  scf_dfa_is_comma, _function_action_comma);
	SCF_DFA_MODULE_NODE(dfa, function, end,    scf_dfa_is_entry, _function_action_end);

	SCF_DFA_MODULE_NODE(dfa, function, lp,     scf_dfa_is_lp,    _function_action_lp);
	SCF_DFA_MODULE_NODE(dfa, function, rp,     scf_dfa_is_rp,    _function_action_rp);

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	dfa_fun_data_t*   fd    = d->module_datas[dfa_module_function.index];

	assert(!fd);

	fd = calloc(1, sizeof(dfa_fun_data_t));
	if (!fd) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_function.index] = fd;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_function(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	dfa_fun_data_t*   fd    = d->module_datas[dfa_module_function.index];

	if (fd) {
		free(fd);
		fd = NULL;
		d->module_datas[dfa_module_function.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_function(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, function, comma,     comma);

	SCF_DFA_GET_MODULE_NODE(dfa, function, lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, function, rp,        rp);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     base_type, base_type);
	SCF_DFA_GET_MODULE_NODE(dfa, identity, identity,  type_name);

	SCF_DFA_GET_MODULE_NODE(dfa, type,     star,      star);
	SCF_DFA_GET_MODULE_NODE(dfa, type,     identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, block,    entry,     block);

	// function start
	scf_dfa_node_add_child(identity,  lp);

	// function args
	scf_dfa_node_add_child(lp,        base_type);
	scf_dfa_node_add_child(lp,        type_name);
	scf_dfa_node_add_child(lp,        rp);

	scf_dfa_node_add_child(identity,  comma);
	scf_dfa_node_add_child(identity,  rp);

	scf_dfa_node_add_child(base_type, comma);
	scf_dfa_node_add_child(type_name, comma);
	scf_dfa_node_add_child(base_type, rp);
	scf_dfa_node_add_child(type_name, rp);
	scf_dfa_node_add_child(star,      comma);
	scf_dfa_node_add_child(star,      rp);

	scf_dfa_node_add_child(comma,     base_type);
	scf_dfa_node_add_child(comma,     type_name);

	// function body
	scf_dfa_node_add_child(rp,        block);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_function =
{
	.name        = "function",

	.init_module = _dfa_init_module_function,
	.init_syntax = _dfa_init_syntax_function,

	.fini_module = _dfa_fini_module_function,
};

