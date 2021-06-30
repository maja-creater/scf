#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"
#include"scf_stack.h"

extern scf_dfa_module_t dfa_module_for;

typedef struct {
	int              nb_lps;
	int              nb_rps;

	scf_block_t*     parent_block;
	scf_node_t*      parent_node;

	scf_node_t*      _for;

	int              nb_semicolons;
	scf_vector_t*    init_exprs;
	scf_expr_t*      cond_expr;
	scf_vector_t*    update_exprs;

	scf_dfa_hook_t*  hook_end;

} dfa_for_data_t;

static int _for_is_for(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_FOR == w->type;
}

static int _for_is_end(scf_dfa_t* dfa, void* word)
{
	return 1;
}

static int _for_action_for(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];

	scf_node_t*       _for  = scf_node_alloc(w, SCF_OP_FOR, NULL);
	if (!_for) {
		scf_loge("node alloc failed\n");
		return SCF_DFA_ERROR;
	}

	dfa_for_data_t* fd = calloc(1, sizeof(dfa_for_data_t));
	if (!fd) {
		scf_loge("module data alloc failed\n");
		return SCF_DFA_ERROR;
	}

	if (d->current_node)
		scf_node_add_child(d->current_node, _for);
	else {
		scf_node_add_child((scf_node_t*)parse->ast->current_block, _for);
		scf_loge("current_block: %p\n", parse->ast->current_block);
	}

	fd->parent_block = parse->ast->current_block;
	fd->parent_node  = d->current_node;
	fd->_for         = _for;
	d->current_node  = _for;

	scf_stack_push(s, fd);

	return SCF_DFA_NEXT_WORD;
}

static int _for_action_lp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*   fd    = scf_stack_top(s);

	assert(!d->expr);
	d->expr_local_flag = 1;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_rp"),        SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_semicolon"), SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_comma"),     SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_lp_stat"),   SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _for_action_lp_stat(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	dfa_parse_data_t* d  = data;
	scf_stack_t*      s  = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*   fd = scf_stack_top(s);

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_lp_stat"), SCF_DFA_HOOK_POST);

	fd->nb_lps++;

	return SCF_DFA_NEXT_WORD;
}

static int _for_action_comma(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*   fd    = scf_stack_top(s);

	if (!d->expr) {
		scf_loge("need expr before ','\n");
		return SCF_DFA_ERROR;
	}

	if (0 == fd->nb_semicolons) {
		if (!fd->init_exprs)
			fd->init_exprs = scf_vector_alloc();

		scf_vector_add(fd->init_exprs, d->expr);
		d->expr = NULL;

	} else if (1 == fd->nb_semicolons) {
		fd->cond_expr = d->expr;
		d->expr = NULL;

	} else if (2 == fd->nb_semicolons) {
		if (!fd->update_exprs)
			fd->update_exprs = scf_vector_alloc();

		scf_vector_add(fd->update_exprs, d->expr);
		d->expr = NULL;
	} else {
		scf_loge("too many ';' in for\n");
		return SCF_DFA_ERROR;
	}

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_comma"),   SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_lp_stat"), SCF_DFA_HOOK_POST);

	return SCF_DFA_NEXT_WORD;
}

static int _for_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	printf("%s(),%d\n", __func__, __LINE__);
	if (!data) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*   fd    = scf_stack_top(s);

	if (0 == fd->nb_semicolons) {
		if (d->expr) {
			if (!fd->init_exprs)
				fd->init_exprs = scf_vector_alloc();

			scf_vector_add(fd->init_exprs, d->expr);
			d->expr = NULL;
		}
	} else if (1 == fd->nb_semicolons) {
		if (d->expr) {
			fd->cond_expr = d->expr;
			d->expr = NULL;
		}
	} else {
		scf_loge("too many ';' in for\n");
		return SCF_DFA_ERROR;
	}

	fd->nb_semicolons++;

	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_semicolon"), SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_comma"),     SCF_DFA_HOOK_POST);
	SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_lp_stat"),   SCF_DFA_HOOK_POST);

	return SCF_DFA_SWITCH_TO;
}

static int _for_add_expr_vector(dfa_for_data_t* fd, scf_vector_t* vec)
{
	if (!vec) {
		scf_node_add_child(fd->_for, NULL);
		return SCF_DFA_OK;
	}

	if (0 == vec->size) {
		scf_node_add_child(fd->_for, NULL);

		scf_vector_free(vec);
		vec = NULL;
		return SCF_DFA_OK;
	}

	scf_node_t* parent = fd->_for;
	if (vec->size > 1) {

		scf_block_t* b = scf_block_alloc_cstr("for");

		scf_node_add_child(fd->_for, (scf_node_t*)b);
		parent = (scf_node_t*)b;
	}

	int i;
	for (i = 0; i < vec->size; i++) {

		scf_expr_t* e = vec->data[i];

		scf_node_add_child(parent, e);
	}

	scf_vector_free(vec);
	vec = NULL;
	return SCF_DFA_OK;
}

static int _for_add_exprs(dfa_for_data_t* fd)
{
	_for_add_expr_vector(fd, fd->init_exprs);
	fd->init_exprs = NULL;

	scf_node_add_child(fd->_for, fd->cond_expr);
	fd->cond_expr = NULL;

	_for_add_expr_vector(fd, fd->update_exprs);
	fd->update_exprs = NULL;

	return SCF_DFA_OK;
}

static int _for_action_rp(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = data;
	scf_lex_word_t*   w     = words->data[words->size - 1];
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*  fd     = scf_stack_top(s);

	fd->nb_rps++;

	if (fd->nb_rps < fd->nb_lps) {

		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_rp"),        SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_semicolon"), SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_comma"),     SCF_DFA_HOOK_POST);
		SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_lp_stat"),   SCF_DFA_HOOK_POST);

		return SCF_DFA_NEXT_WORD;
	}

	if (2 == fd->nb_semicolons) {
		if (!fd->update_exprs)
			fd->update_exprs = scf_vector_alloc();

		scf_vector_add(fd->update_exprs, d->expr);
		d->expr = NULL;
	} else {
		scf_loge("too many ';' in for\n");
		return SCF_DFA_ERROR;
	}

	_for_add_exprs(fd);
	d->expr_local_flag = 0;

	fd->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "for_end"), SCF_DFA_HOOK_END);

	return SCF_DFA_SWITCH_TO;
}

static int _for_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	scf_parse_t*      parse     = dfa->priv;
	dfa_parse_data_t* d         = data;
	scf_stack_t*      s         = d->module_datas[dfa_module_for.index];
	dfa_for_data_t*  fd         = scf_stack_pop(s);

	scf_loge("current_block: %p, parent_block: %p\n", parse->ast->current_block, fd->parent_block);

	assert(parse->ast->current_block == fd->parent_block);

	d->current_node = fd->parent_node;

	scf_logi("\033[31m for: %d, fd: %p, hook_end: %p, s->size: %d\033[0m\n",
			fd->_for->w->line, fd, fd->hook_end, s->size);

	free(fd);
	fd = NULL;

	assert(s->size >= 0);

	return SCF_DFA_OK;
}

static int _dfa_init_module_for(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, for, semicolon, scf_dfa_is_semicolon,  _for_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, for, comma,     scf_dfa_is_comma,      _for_action_comma);
	SCF_DFA_MODULE_NODE(dfa, for, end,       _for_is_end,           _for_action_end);

	SCF_DFA_MODULE_NODE(dfa, for, lp,        scf_dfa_is_lp,         _for_action_lp);
	SCF_DFA_MODULE_NODE(dfa, for, lp_stat,   scf_dfa_is_lp,         _for_action_lp_stat);
	SCF_DFA_MODULE_NODE(dfa, for, rp,        scf_dfa_is_rp,         _for_action_rp);

	SCF_DFA_MODULE_NODE(dfa, for, _for,      _for_is_for,           _for_action_for);

	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];

	assert(!s);

	s = scf_stack_alloc();
	if (!s) {
		scf_loge("\n");
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_for.index] = s;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_for(scf_dfa_t* dfa)
{
	scf_parse_t*      parse = dfa->priv;
	dfa_parse_data_t* d     = parse->dfa_data;
	scf_stack_t*      s     = d->module_datas[dfa_module_for.index];

	if (s) {
		scf_stack_free(s);
		s = NULL;
		d->module_datas[dfa_module_for.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_for(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, for,   semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   comma,     comma);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   lp,        lp);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   lp_stat,   lp_stat);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   rp,        rp);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   _for,    _for);
	SCF_DFA_GET_MODULE_NODE(dfa, for,   end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa, expr,  entry,     expr);
	SCF_DFA_GET_MODULE_NODE(dfa, block, entry,     block);

	// for start
	scf_vector_add(dfa->syntaxes,  _for);

	// condition expr
	scf_dfa_node_add_child(_for,      lp);
	scf_dfa_node_add_child(lp,        semicolon);
	scf_dfa_node_add_child(semicolon, semicolon);
	scf_dfa_node_add_child(semicolon, rp);

	scf_dfa_node_add_child(lp,        expr);
	scf_dfa_node_add_child(expr,      semicolon);
	scf_dfa_node_add_child(semicolon, expr);
	scf_dfa_node_add_child(expr,      rp);

	// for body block
	scf_dfa_node_add_child(rp,     block);

	scf_logi("\n");
	return 0;
}

scf_dfa_module_t dfa_module_for =
{
	.name        = "for",

	.init_module = _dfa_init_module_for,
	.init_syntax = _dfa_init_syntax_for,

	.fini_module = _dfa_fini_module_for,
};

