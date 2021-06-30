#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_block;

typedef struct {

	int              nb_lbs;
	int              nb_rbs;

	scf_block_t*     parent_block;

	scf_node_t*      parent_node;

	scf_dfa_hook_t*  hook_end;

} dfa_block_data_t;

static int _block_action_entry(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse     = dfa->priv;
	dfa_parse_data_t* d         = data;
	scf_stack_t*      s         = d->module_datas[dfa_module_block.index];
	dfa_block_data_t* bd        = scf_stack_top(s);

	if (!bd) {
		dfa_block_data_t* bd = calloc(1, sizeof(dfa_block_data_t));
		assert(bd);

		bd->parent_block = parse->ast->current_block;
		bd->parent_node  = d->current_node;

		bd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "block_end"), SCF_DFA_HOOK_END);
		assert(bd->hook_end);

		scf_stack_push(s, bd);

		scf_logi("\033[31m new bd: %p, s->size: %d\033[0m\n", bd, s->size);
	} else {
		scf_logi("\033[31m new bd: %p, s->size: %d\033[0m\n", bd, s->size);
	}

	return words->size > 0 ? SCF_DFA_CONTINUE : SCF_DFA_NEXT_WORD;
}

static int _block_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse     = dfa->priv;
	dfa_parse_data_t* d         = data;
	scf_stack_t*      s         = d->module_datas[dfa_module_block.index];
	dfa_block_data_t* bd        = scf_stack_top(s);

	if (bd->nb_rbs < bd->nb_lbs) {
		scf_logi("\033[31m end bd: %p, bd->nb_lbs: %d, bd->nb_rbs: %d, s->size: %d\033[0m\n",
				bd, bd->nb_lbs, bd->nb_rbs, s->size);

		bd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "block_end"), SCF_DFA_HOOK_END);
		assert(bd->hook_end);

		return SCF_DFA_SWITCH_TO;
	}

	assert(bd->nb_lbs       == bd->nb_rbs);
	assert(bd->parent_block == parse->ast->current_block);
	assert(bd->parent_node  == d->current_node);

	scf_stack_pop(s);

	scf_logi("\033[31m end bd: %p, bd->nb_lbs: %d, bd->nb_rbs: %d, s->size: %d\033[0m\n",
			bd, bd->nb_lbs, bd->nb_rbs, s->size);

	free(bd);
	bd = NULL;

	return SCF_DFA_OK;
}

static int _block_action_lb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_block.index];

	dfa_block_data_t* bd = calloc(1, sizeof(dfa_block_data_t));
	assert(bd);

	scf_block_t* b = scf_block_alloc(w);
	assert(b);

	if (d->current_node)
		scf_node_add_child(d->current_node, (scf_node_t*)b);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)b);

	bd->parent_block = parse->ast->current_block;
	bd->parent_node  = d->current_node;

	parse->ast->current_block = b;
	d->current_node = NULL;

	bd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "block_end"), SCF_DFA_HOOK_END);
	assert(bd->hook_end);

	bd->nb_lbs++;
	scf_stack_push(s, bd);

	scf_logi("\033[31m new bd: %p, s->size: %d\033[0m\n", bd, s->size);

	return SCF_DFA_NEXT_WORD;
}

static int _block_action_rb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_block.index];
	dfa_block_data_t* bd    = scf_stack_top(s);

	bd->nb_rbs++;

	scf_logi("\033[31m bd: %p, bd->nb_lbs: %d, bd->nb_rbs: %d, s->size: %d\033[0m\n",
			bd, bd->nb_lbs, bd->nb_rbs, s->size);

	assert(bd->nb_lbs       == bd->nb_rbs);

	parse->ast->current_block = bd->parent_block;
	d->current_node = bd->parent_node;

	return SCF_DFA_OK;
}

static int _dfa_init_module_block(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, block, entry, scf_dfa_is_entry, _block_action_entry);
	SCF_DFA_MODULE_NODE(dfa, block, end,   scf_dfa_is_entry, _block_action_end);
	SCF_DFA_MODULE_NODE(dfa, block, lb,    scf_dfa_is_lb,    _block_action_lb);
	SCF_DFA_MODULE_NODE(dfa, block, rb,    scf_dfa_is_rb,    _block_action_rb);

	SCF_DFA_GET_MODULE_NODE(dfa, block,     entry,     entry);
	SCF_DFA_GET_MODULE_NODE(dfa, block,     end,       end);
	SCF_DFA_GET_MODULE_NODE(dfa, block,     lb,        lb);
	SCF_DFA_GET_MODULE_NODE(dfa, block,     rb,        rb);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,      entry,     expr);
	SCF_DFA_GET_MODULE_NODE(dfa, type,      entry,     type);

	SCF_DFA_GET_MODULE_NODE(dfa, if,       _if,       _if);
	SCF_DFA_GET_MODULE_NODE(dfa, while,    _while,    _while);
	SCF_DFA_GET_MODULE_NODE(dfa, for,      _for,      _for);

	SCF_DFA_GET_MODULE_NODE(dfa, break,    _break,    _break);
	SCF_DFA_GET_MODULE_NODE(dfa, continue, _continue, _continue);
	SCF_DFA_GET_MODULE_NODE(dfa, return,   _return,   _return);
	SCF_DFA_GET_MODULE_NODE(dfa, goto,     _goto,     _goto);
	SCF_DFA_GET_MODULE_NODE(dfa, label,    label,     label);
	SCF_DFA_GET_MODULE_NODE(dfa, error,    error,     error);
	SCF_DFA_GET_MODULE_NODE(dfa, async,    async,     async);

	SCF_DFA_GET_MODULE_NODE(dfa, va_arg,   start,     va_start);
	SCF_DFA_GET_MODULE_NODE(dfa, va_arg,   end,       va_end);

	// block could includes these statements
	scf_dfa_node_add_child(entry, lb);
	scf_dfa_node_add_child(entry, rb);

	scf_dfa_node_add_child(entry, va_start);
	scf_dfa_node_add_child(entry, va_end);
	scf_dfa_node_add_child(entry, expr);
	scf_dfa_node_add_child(entry, type);

	scf_dfa_node_add_child(entry, _if);
	scf_dfa_node_add_child(entry, _while);
	scf_dfa_node_add_child(entry, _for);

	scf_dfa_node_add_child(entry, _break);
	scf_dfa_node_add_child(entry, _continue);
	scf_dfa_node_add_child(entry, _return);
	scf_dfa_node_add_child(entry, _goto);
	scf_dfa_node_add_child(entry, label);
	scf_dfa_node_add_child(entry, error);
	scf_dfa_node_add_child(entry, async);


	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_block.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("error: \n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_block.index] = s;
	return SCF_DFA_OK;
}

static int _dfa_fini_module_block(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_block.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_block.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_block(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, block,     entry,     entry);
	SCF_DFA_GET_MODULE_NODE(dfa, block,     end,       end);

	scf_dfa_node_add_child(entry, end);
	scf_dfa_node_add_child(end,   entry);
	//scf_dfa_node_add_child(entry, entry);

	int i;
	for (i = 0; i < entry->childs->size; i++) {
		scf_dfa_node_t* n = entry->childs->data[i];

		scf_logi("n->name: %s\n", n->name);
	}

	return 0;
}

scf_dfa_module_t dfa_module_block =
{
	.name        = "block",
	.init_module = _dfa_init_module_block,
	.init_syntax = _dfa_init_syntax_block,

	.fini_module = _dfa_fini_module_block,
};

