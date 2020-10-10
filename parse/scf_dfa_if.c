#include"scf_dfa.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_if;

typedef struct {
	int              nb_lps;
	int              nb_rps;

	scf_block_t*     parent_block;
	scf_node_t*      parent_node;

	scf_node_t*      _if;

	scf_lex_word_t*  prev_else;
	scf_lex_word_t*  next_else;

	scf_dfa_hook_t*  hook_end;

} dfa_if_data_t;

static int _if_is_lp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_LP == w->type;
}

static int _if_is_rp(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_RP == w->type;
}

static int _if_is_if(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_IF == w->type;
}

static int _if_is_else(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_ELSE == w->type;
}

static int _if_is_end(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_ELSE != w->type;
}

static int _if_action_if(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd   = scf_stack_top(s);

	scf_node_t*       _if   = scf_node_alloc(w, SCF_OP_IF, NULL);
	if (!_if) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	dfa_if_data_t* ifd2 = calloc(1, sizeof(dfa_if_data_t));
	if (!ifd2) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _if);
	else
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _if);

	ifd2->parent_block = parse->ast->current_block;
	ifd2->parent_node  = d->current_node;
	ifd2->_if          = _if;

	if (ifd && ifd->next_else)
		ifd2->prev_else = ifd->next_else;

	scf_stack_push(s, ifd2);

	d->current_node = _if;

	return SCF_DFA_NEXT_WORD;
}

static int _if_action_else(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd   = scf_stack_top(s);

	if (!ifd) {
		scf_loge("no 'if' before 'else' in line: %d\n", w->line);
		return SCF_DFA_ERROR;
	}

	if (ifd->next_else) {
		scf_loge("continuous 2 'else', 1st line: %d, 2nd line: %d\n", ifd->next_else->line, w->line);
		return SCF_DFA_ERROR;
	}

	scf_logd("ifd->_if->nb_nodes: %d, line: %d\n", ifd->_if->nb_nodes, ifd->_if->w->line);
	assert(2 == ifd->_if->nb_nodes);

	ifd->next_else = w;

	assert(!d->expr);
	d->current_node = ifd->_if;

	return SCF_DFA_NEXT_WORD;
}

static int _if_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd   = scf_stack_top(s);

	if (d->expr) {
		scf_expr_free(d->expr);
		d->expr = NULL;
	}
	d->expr_local_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_lp_stat"), SCF_DFA_HOOK_POST);

	scf_logd("ifd->nb_lps: %d, ifd->nb_rps: %d\n", ifd->nb_lps, ifd->nb_rps);

	return SCF_DFA_NEXT_WORD;
}

static int _if_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd   = scf_stack_top(s);

	ifd->nb_lps++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _if_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d     = data;
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd   = scf_stack_top(s);

	if (!d->expr) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	ifd->nb_rps++;

	if (ifd->nb_rps == ifd->nb_lps) {

		assert(0 == ifd->_if->nb_nodes);

		scf_node_add_child(ifd->_if, d->expr);
		d->expr = NULL;

		d->expr_local_flag = 0;

		ifd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_end"), SCF_DFA_HOOK_END);
		return SCF_DFA_SWITCH_TO;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_rp"),      SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _is_end(scf_dfa_t* dfa)
{
	scf_lex_word_t* w   = dfa->ops->pop_word(dfa);
	int             ret = SCF_LEX_WORD_KEY_ELSE != w->type;

	dfa->ops->push_word(dfa, w);

	return ret;
}

static int _if_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse     = dfa->priv;
	dfa_parse_data_t* d         = data;
	scf_stack_t*      s         = d->module_datas[dfa_module_if.index];
	dfa_if_data_t*    ifd       = scf_stack_top(s);
	scf_lex_word_t*   prev_else = ifd->prev_else;

	if (!_is_end(dfa)) {

		if (3 == ifd->_if->nb_nodes) {
			if (1 == s->size)
				return SCF_DFA_NEXT_WORD;
		} else {
			ifd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "if_end"), SCF_DFA_HOOK_END);
			return SCF_DFA_NEXT_WORD;
		}
	}

	assert(parse->ast->current_block == ifd->parent_block);

	scf_stack_pop(s);

	d->current_node = ifd->parent_node;

	scf_logi("\033[31m if: %d, ifd: %p, hook_end: %p, s->size: %d\033[0m\n",
			ifd->_if->w->line, ifd, ifd->hook_end, s->size);

	free(ifd);
	ifd = NULL;

	assert(s->size >= 0);

	return SCF_DFA_OK;
}

static int _dfa_init_module_if(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, if, end,       _if_is_end,       _if_action_end);

	SCF_DFA_MODULE_NODE(dfa, if, lp,        _if_is_lp,        _if_action_lp);
	SCF_DFA_MODULE_NODE(dfa, if, rp,        _if_is_rp,        _if_action_rp);
	SCF_DFA_MODULE_NODE(dfa, if, lp_stat,   _if_is_lp,        _if_action_lp_stat);

	SCF_DFA_MODULE_NODE(dfa, if, _if,       _if_is_if,        _if_action_if);
	SCF_DFA_MODULE_NODE(dfa, if, _else,     _if_is_else,      _if_action_else);

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_if.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_if(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_if.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_if.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_if(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, if,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, if,   rp,        rp);
	SCF_DFA_GET_MODULE_NODE(dfa, if,   lp_stat,   lp_stat);
	SCF_DFA_GET_MODULE_NODE(dfa, if,   _if,       _if);
	SCF_DFA_GET_MODULE_NODE(dfa, if,   _else,     _else);
	SCF_DFA_GET_MODULE_NODE(dfa, if,   end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,  entry,     expr);
	SCF_DFA_GET_MODULE_NODE(dfa, block, entry,     block);

	// if start
	scf_vector_add(dfa->syntaxes,  _if);

	// condition expr
	scf_dfa_node_add_child(_if,    lp);
	scf_dfa_node_add_child(lp,     expr);
	scf_dfa_node_add_child(expr,   rp);

	// if body block
	scf_dfa_node_add_child(rp,     block);

	// recursive else if block
	scf_dfa_node_add_child(block,  _else);
	scf_dfa_node_add_child(_else,  _if);

	// last else block
	scf_dfa_node_add_child(_else,  block);

	scf_loge("\n");
	return 0;
}

scf_dfa_module_t dfa_module_if =
{
	.name        = "if",

	.init_module = _dfa_init_module_if,
	.init_syntax = _dfa_init_syntax_if,

	.fini_module = _dfa_fini_module_if,
};

