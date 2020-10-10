#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_while;

typedef struct {
	int              nb_lps;
	int              nb_rps;

	scf_block_t*     parent_block;
	scf_node_t*      parent_node;

	scf_node_t*      _while;

	scf_dfa_hook_t*  hook_end;

} dfa_while_data_t;

static int _while_is_while(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_WHILE == w->type;
}

static int _while_is_end(scf_dfa_t* dfa, void* word)
{
	return 1;
}

static int _while_action_while(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];

	scf_node_t*       _while   = scf_node_alloc(w, SCF_OP_WHILE, NULL);
	if (!_while) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	dfa_while_data_t* wd = calloc(1, sizeof(dfa_while_data_t));
	if (!wd) {
		scf_loge("module data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _while);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _while);

	wd->_while       = _while;
	wd->parent_block = parse->ast->current_block;
	wd->parent_node  = d->current_node;
	d->current_node  = _while;

	scf_stack_push(s, wd);

	return SCF_DFA_NEXT_WORD;
}

static int _while_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];
	dfa_while_data_t* wd    = scf_stack_top(s);

	assert(!d->expr);
	d->expr_local_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _while_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];
	dfa_while_data_t* wd    = scf_stack_top(s);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_lp_stat"), SCF_DFA_HOOK_POST);

	wd->nb_lps++;

	return SCF_DFA_NEXT_WORD;
}

static int _while_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];
	dfa_while_data_t* wd    = scf_stack_top(s);

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	wd->nb_rps++;

	if (wd->nb_rps == wd->nb_lps) {

		assert(0 == wd->_while->nb_nodes);

		scf_node_add_child(wd->_while, d->expr);
		d->expr = NULL;

		d->expr_local_flag = 0;

		wd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_end"), SCF_DFA_HOOK_END);

		return SCF_DFA_SWITCH_TO;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "while_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _while_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse     = dfa->priv;
	dfa_parse_data_t* d         = data;
	scf_stack_t*      s         = d->module_datas[dfa_module_while.index];
	dfa_while_data_t* wd        = scf_stack_pop(s);

	assert(parse->ast->current_block == wd->parent_block);

	d->current_node = wd->parent_node;

	scf_logi("\033[31m while: %d, wd: %p, hook_end: %p, s->size: %d\033[0m\n",
			wd->_while->w->line,
			wd, wd->hook_end, s->size);

	free(wd);
	wd = NULL;

	assert(s->size >= 0);

	return SCF_DFA_OK;
}

static int _dfa_init_module_while(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, while, end,       _while_is_end,    _while_action_end);

	SCF_DFA_MODULE_NODE(dfa, while, lp,        scf_dfa_is_lp,    _while_action_lp);
	SCF_DFA_MODULE_NODE(dfa, while, rp,        scf_dfa_is_rp,    _while_action_rp);
	SCF_DFA_MODULE_NODE(dfa, while, lp_stat,   scf_dfa_is_lp,    _while_action_lp_stat);

	SCF_DFA_MODULE_NODE(dfa, while, _while,    _while_is_while,  _while_action_while);

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_logi("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_while.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_while(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_while.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_while.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_while(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, while,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, while,   rp,        rp);
	SCF_DFA_GET_MODULE_NODE(dfa, while,   lp_stat,   lp_stat);
	SCF_DFA_GET_MODULE_NODE(dfa, while,   _while,    _while);
	SCF_DFA_GET_MODULE_NODE(dfa, while,   end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,  entry,     expr);
	SCF_DFA_GET_MODULE_NODE(dfa, block, entry,     block);

	// while start
	scf_vector_add(dfa->syntaxes,  _while);

	// condition expr
	scf_dfa_node_add_child(_while, lp);
	scf_dfa_node_add_child(lp,     expr);
	scf_dfa_node_add_child(expr,   rp);

	// while body
	scf_dfa_node_add_child(rp,     block);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_while =
{
	.name        = "while",

	.init_module = _dfa_init_module_while,
	.init_syntax = _dfa_init_syntax_while,

	.fini_module = _dfa_fini_module_while,
};

