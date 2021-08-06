#include"scf_dfa.h"
#include"scf_dfa_util.h"
#include"scf_parse.h"

extern scf_dfa_module_t dfa_module_class;

typedef struct {
	scf_lex_word_t*  current_identity;

	scf_block_t*     parent_block;

	scf_type_t*      current_class;

	scf_dfa_hook_t*  hook_end;

	int              nb_lbs;
	int              nb_rbs;

} class_module_data_t;

static int _class_is_class(scf_dfa_t* dfa, void* word)
{
	scf_lex_word_t* w = word;

	return SCF_LEX_WORD_KEY_CLASS  == w->type
		|| SCF_LEX_WORD_KEY_STRUCT == w->type;
}

static int _class_action_identity(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (!data) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];
	scf_lex_word_t*       w     = words->data[words->size - 1];

	if (md->current_identity) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_type_t* t = scf_block_find_type(parse->ast->current_block, w->text->data);
	if (!t) {
		t = scf_type_alloc(w, w->text->data, SCF_STRUCT + parse->ast->nb_structs, 0);
		if (!t) {
			printf("%s(), %d, error: \n", __func__, __LINE__);
			return SCF_DFA_ERROR;
		}

		parse->ast->nb_structs++;
		t->node.class_flag = 1;
		scf_scope_push_type(parse->ast->current_block->scope, t);
		scf_node_add_child((scf_node_t*)parse->ast->current_block, (scf_node_t*)t);
	}

	md->current_identity = w;
	md->parent_block     = parse->ast->current_block;
	md->hook_end         = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "class_end"), SCF_DFA_HOOK_END);

	printf("\033[31m%s(), %d, t: %p, t->type: %d\033[0m\n", __func__, __LINE__, t, t->type);

	return SCF_DFA_NEXT_WORD;
}

static int _class_action_lb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (!data) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];
	scf_lex_word_t*       w     = words->data[words->size - 1];

	if (!md->current_identity) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_type_t* t = scf_block_find_type(parse->ast->current_block, md->current_identity->text->data);
	if (!t) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	if (!t->scope)
		t->scope = scf_scope_alloc(w, "class");

	md->parent_block  = parse->ast->current_block;
	md->current_class = t;
	md->nb_lbs++;

	parse->ast->current_block = (scf_block_t*)t;

	printf("%s(), %d, t: %p\n", __func__, __LINE__, t);

	return SCF_DFA_NEXT_WORD;
}

static int _class_calculate_size(scf_dfa_t* dfa, scf_type_t* s)
{
	scf_parse_t* parse = dfa->priv;

	int size = 0;
	int i;
	for (i = 0; i < s->scope->vars->size; i++) {

		scf_variable_t* v = s->scope->vars->data[i];

		if (v->size < 0) {
			printf("%s(), %d, error: sizeof var: '%s'\n", __func__, __LINE__, v->w->text->data);
			return SCF_DFA_ERROR;
		}

		int align;
		if (1 == v->size)
			align = 1;
		else if (2 == v->size)
			align = 2;
		else if (v->size <= 4)
			align = 4;
		else
			align = 8;

		v->offset = (size + align - 1) / align * align;

		if (v->nb_dimentions > 0) {
			int j;
			int capacity = 1;

			for (j = 0; j < v->nb_dimentions; j++) {
				if (v->dimentions[j] < 0) {

					printf("%s(), %d, error: v: '%s'\n", __func__, __LINE__, v->w->text->data);
					return SCF_DFA_ERROR;
				}

				capacity *= v->dimentions[j];
			}

			v->capacity = capacity;
			size = v->offset + v->size * v->capacity;
		} else {
			size = v->offset + v->size;
		}

		printf("%s(), %d, class '%s', member: '%s', offset: %d, size: %d, v->dim: %d, v->capacity: %d\n", __func__, __LINE__,
				s->name->data, v->w->text->data, v->offset, v->size, v->nb_dimentions, v->capacity);
	}
	s->size = size;
	s->node.define_flag = 1;

	printf("%s(), %d, class '%s', size: %d\n", __func__, __LINE__, s->name->data, s->size);
	return 0;
}

static int _class_action_rb(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	printf("%s(), %d\n", __func__, __LINE__);

	if (!data) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];

	if (_class_calculate_size(dfa, md->current_class) < 0) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	md->nb_rbs++;

	return SCF_DFA_NEXT_WORD;
}

static int _class_action_semicolon(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	printf("%s(), %d\n", __func__, __LINE__);

	return SCF_DFA_OK;
}

static int _class_action_end(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	if (!data) {
		printf("%s(), %d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];

	if (md->nb_rbs == md->nb_lbs) {
		printf("%s(), %d, SCF_DFA_OK\n", __func__, __LINE__);

		parse->ast->current_block = md->parent_block;

		md->current_identity = NULL;
		md->current_class    = NULL;
		md->parent_block     = NULL;
		md->nb_lbs           = 0;
		md->nb_rbs           = 0;
		md->hook_end         = NULL;

		return SCF_DFA_OK;
	}

	md->hook_end = SCF_DFA_PUSH_HOOK(scf_dfa_find_node(dfa, "class_end"), SCF_DFA_HOOK_END);

	printf("%s(), %d\n", __func__, __LINE__);
	return SCF_DFA_SWITCH_TO;
}

static int _dfa_init_module_class(scf_dfa_t* dfa)
{
	SCF_DFA_MODULE_NODE(dfa, class, _class,    _class_is_class,      NULL);

	SCF_DFA_MODULE_NODE(dfa, class, identity,  scf_dfa_is_identity,  _class_action_identity);

	SCF_DFA_MODULE_NODE(dfa, class, lb,        scf_dfa_is_lb,        _class_action_lb);
	SCF_DFA_MODULE_NODE(dfa, class, rb,        scf_dfa_is_rb,        _class_action_rb);

	SCF_DFA_MODULE_NODE(dfa, class, semicolon, scf_dfa_is_semicolon, _class_action_semicolon);
	SCF_DFA_MODULE_NODE(dfa, class, end,       scf_dfa_is_entry,     _class_action_end);

	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = parse->dfa_data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];

	assert(!md);

	md = calloc(1, sizeof(class_module_data_t));
	if (!md) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return SCF_DFA_ERROR;
	}

	d->module_datas[dfa_module_class.index] = md;

	return SCF_DFA_OK;
}

static int _dfa_fini_module_class(scf_dfa_t* dfa)
{
	scf_parse_t*          parse = dfa->priv;
	dfa_parse_data_t*     d     = parse->dfa_data;
	class_module_data_t*  md    = d->module_datas[dfa_module_class.index];

	if (md) {
		free(md);
		md = NULL;
		d->module_datas[dfa_module_class.index] = NULL;
	}

	return SCF_DFA_OK;
}

static int _dfa_init_syntax_class(scf_dfa_t* dfa)
{
	SCF_DFA_GET_MODULE_NODE(dfa, class,  _class,    _class);
	SCF_DFA_GET_MODULE_NODE(dfa, class,  identity,  identity);
	SCF_DFA_GET_MODULE_NODE(dfa, class,  lb,        lb);
	SCF_DFA_GET_MODULE_NODE(dfa, class,  rb,        rb);
	SCF_DFA_GET_MODULE_NODE(dfa, class,  semicolon, semicolon);
	SCF_DFA_GET_MODULE_NODE(dfa, class,  end,       end);

	SCF_DFA_GET_MODULE_NODE(dfa, type,   entry,     member);
	SCF_DFA_GET_MODULE_NODE(dfa, union,  _union,    _union);

	scf_vector_add(dfa->syntaxes,     _class);

	// class start
	scf_dfa_node_add_child(_class,    identity);

	scf_dfa_node_add_child(identity,  semicolon);
	scf_dfa_node_add_child(identity,  lb);

	scf_dfa_node_add_child(lb,        rb);

	scf_dfa_node_add_child(lb,        _union);
	scf_dfa_node_add_child(lb,        member);
	scf_dfa_node_add_child(member,    rb);
	scf_dfa_node_add_child(rb,        semicolon);

	scf_dfa_node_add_child(end,       _union);
	scf_dfa_node_add_child(end,       member);
	scf_dfa_node_add_child(end,       rb);

	printf("%s(),%d\n\n", __func__, __LINE__);
	return 0;
}

scf_dfa_module_t dfa_module_class = 
{
	.name        = "class",
	.init_module = _dfa_init_module_class,
	.init_syntax = _dfa_init_syntax_class,

	.fini_module = _dfa_fini_module_class,
};

