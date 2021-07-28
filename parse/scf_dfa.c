#include"scf_dfa.h"
#include"scf_lex_word.h"
#include<unistd.h>

extern scf_dfa_ops_t   dfa_ops_parse;

static scf_dfa_ops_t*  dfa_ops_array[] = 
{
	&dfa_ops_parse,

	NULL,
};

static int _scf_dfa_node_parse_word(scf_dfa_t* dfa, scf_dfa_node_t* node, scf_vector_t* words, void* data);

void scf_dfa_del_hook_by_name(scf_dfa_hook_t** pp, const char* name)
{
	while (*pp) {
		scf_dfa_hook_t* hook = *pp;

		if (!strcmp(name, hook->node->name)) {
			*pp = hook->next;
			free(hook);
			hook = NULL;
			continue;
		}

		pp = &hook->next;
	}
}

void scf_dfa_del_hook(scf_dfa_hook_t** pp, scf_dfa_hook_t* sentinel)
{
	while (*pp && *pp != sentinel)
		pp = &(*pp)->next;

	if (*pp) {
		*pp = sentinel->next;
		free(sentinel);
		sentinel = NULL;
	}
}

void scf_dfa_clear_hooks(scf_dfa_hook_t** pp, scf_dfa_hook_t* sentinel)
{
	while (*pp != sentinel) {
		scf_dfa_hook_t* h = *pp;

		*pp = h->next;

		free(h);
		h = NULL;
	}
}

scf_dfa_hook_t* scf_dfa_find_hook(scf_dfa_t* dfa, scf_dfa_hook_t** pp, void* word)
{
	while (*pp) {
		scf_dfa_hook_t* h = *pp;

		if (!h->node || !h->node->is) {
			scf_logd("delete invalid hook: %p\n", h);

			*pp = h->next;
			free(h);
			h = NULL;
			continue;
		}

		if (h->node->is(dfa, word)) {
			return h;
		}
		pp = &h->next;
	}

	return NULL;
}

scf_dfa_node_t* scf_dfa_node_alloc(const char* name, scf_dfa_is_pt is, scf_dfa_action_pt action)
{
	if (!name || !is) {
		return NULL;
	}

	scf_dfa_node_t* node = calloc(1, sizeof(scf_dfa_node_t));
	if (!node) {
		return NULL;
	}

	node->name = strdup(name);
	if (!node->name) {
		free(node);
		node = NULL;
		return NULL;
	}

	node->module_index = -1;
	node->is           = is;
	node->action       = action;
	node->refs         = 1;
	return node;
}

void scf_dfa_node_free(scf_dfa_node_t* node)
{
	if (!node)
		return;

	if (--node->refs > 0)
		return;

	assert(0 == node->refs);

	if (node->childs) {
		int i;

		for (i = 0; i < node->childs->size; i++) {
			scf_dfa_node_t* child = node->childs->data[i];

			scf_dfa_node_free(child);
			child = NULL;
		}

		scf_vector_free(node->childs);
		node->childs = NULL;
	}

	free(node->name);
	free(node);
	node = NULL;
}

int scf_dfa_node_add_child(scf_dfa_node_t* parent, scf_dfa_node_t* child)
{
	if (!parent || !child) {
		scf_loge("\n");
		return -1;
	}

	if (!parent->childs) {
		parent->childs = scf_vector_alloc();
		scf_vector_add(parent->childs, child);
		child->refs++;
		return 0;
	}

	int i;

	for (i = 0; i < parent->childs->size; i++) {
		scf_dfa_node_t* node = parent->childs->data[i];

		if (!strcmp(child->name, node->name)) {
			scf_logi("repeated: node->name: %s\n", node->name);
			return SCF_DFA_REPEATED;
		}
	}

	scf_vector_add(parent->childs, child);
	child->refs++;
	return 0;
}

int scf_dfa_open(scf_dfa_t** pdfa, const char* name, void* priv)
{
	if (!pdfa || !name) {
		scf_loge("\n");
		return -1;
	}

	scf_dfa_ops_t* ops = NULL;

	int i;
	for (i = 0; dfa_ops_array[i]; i++) {
		if (!strcmp(name, dfa_ops_array[i]->name)) {
			ops = dfa_ops_array[i];
			break;
		}
	}

	if (!ops) {
		scf_loge("\n");
		return -1;
	}

	scf_dfa_t* dfa = calloc(1, sizeof(scf_dfa_t));
	if (!dfa) {
		scf_loge("\n");
		return -1;
	}

	dfa->nodes    = scf_vector_alloc();
	dfa->syntaxes = scf_vector_alloc();

	dfa->priv = priv;
	dfa->ops  = ops;

	*pdfa = dfa;
	return 0;
}

void scf_dfa_close(scf_dfa_t* dfa)
{
	if (!dfa)
		return;

	if (dfa->nodes) {
		int i;

		for (i = 0; i < dfa->nodes->size; i++) {
			scf_dfa_node_t* node = dfa->nodes->data[i];

			scf_dfa_node_free(node);
			node = NULL;
		}

		scf_vector_free(dfa->nodes);
		dfa->nodes = NULL;
	}

	if (dfa->syntaxes) {
		scf_vector_free(dfa->syntaxes);
		dfa->syntaxes = NULL;
	}

	free(dfa);
	dfa = NULL;
}

int scf_dfa_add_node(scf_dfa_t* dfa, scf_dfa_node_t* node)
{
	if (!dfa || !node)
		return -1;

	if (!dfa->nodes)
		dfa->nodes = scf_vector_alloc();

	scf_vector_add(dfa->nodes, node);
	return 0;
}

scf_dfa_node_t* scf_dfa_find_node(scf_dfa_t* dfa, const char* name)
{
	if (!dfa || !name)
		return NULL;

	if (!dfa->nodes)
		return NULL;

	int i;

	for (i = 0; i < dfa->nodes->size; i++) {
		scf_dfa_node_t* node = dfa->nodes->data[i];

		if (!strcmp(name, node->name))
			return node;
	}

	return NULL;
}

static int _scf_dfa_childs_parse_word(scf_dfa_t* dfa, scf_dfa_node_t** childs, int nb_childs, scf_vector_t* words, void* data)
{
	assert(words->size > 0);

	int i = 0;
	while (i < nb_childs) {
		scf_dfa_node_t* child = childs[i];
		scf_lex_word_t* w     = words->data[words->size - 1];

		scf_logd("nb_childs: %d, child: %s, w: %s\n", nb_childs, child->name, w->text->data);

		scf_dfa_hook_t* hook = scf_dfa_find_hook(dfa, &(dfa->hooks[SCF_DFA_HOOK_PRE]), w);
		if (hook) {
			// if pre hook is set, deliver the word to the proper hook node.
			if (hook->node != child) {
				i++;
				continue;
			}

			scf_logi("pre hook: %s\n", hook->node->name);

			// delete all hooks before it, and itself.
			scf_dfa_clear_hooks(&(dfa->hooks[SCF_DFA_HOOK_PRE]), hook->next);
			hook = NULL;

		} else {
			assert(child->is);
			if (!child->is(dfa, w)) {
				i++;
				continue;
			}
		}

		int ret = _scf_dfa_node_parse_word(dfa, child, words, data);

		if (SCF_DFA_OK == ret) {
			return SCF_DFA_OK;

		} else if (SCF_DFA_ERROR == ret)
			return SCF_DFA_ERROR;

		i++;
	}

	scf_logd("SCF_DFA_NEXT_SYNTAX\n\n");
	return SCF_DFA_NEXT_SYNTAX;
}

static int _scf_dfa_node_parse_word(scf_dfa_t* dfa, scf_dfa_node_t* node, scf_vector_t* words, void* data)
{
	int             ret = SCF_DFA_NEXT_WORD;
	scf_lex_word_t* w   = words->data[words->size - 1];

	scf_logi("\033[33m%s->action(), w: %s, position: %d,%d\033[0m\n", node->name, w->text->data, w->line, w->pos);

	if (node->action) {

		ret = node->action(dfa, words, data);
		if (ret < 0)
			return ret;

		if (SCF_DFA_NEXT_SYNTAX == ret)
			return SCF_DFA_NEXT_SYNTAX;

		if (SCF_DFA_SWITCH_TO == ret)
			ret = SCF_DFA_NEXT_WORD;
	}

	if (SCF_DFA_CONTINUE == ret)
		goto _continue;

#if 1
	scf_dfa_hook_t* h = dfa->hooks[SCF_DFA_HOOK_POST];
	while (h) {
		scf_logd("\033[32m post hook: %s\033[0m\n", h->node->name);
		h = h->next;
	}

	h = dfa->hooks[SCF_DFA_HOOK_END];
	while (h) {
		scf_logd("\033[32m end hook: %s\033[0m\n", h->node->name);
		h = h->next;
	}
	printf("\n");
#endif

	scf_dfa_hook_t* hook = scf_dfa_find_hook(dfa, &(dfa->hooks[SCF_DFA_HOOK_POST]), w);
	if (hook) {
		scf_dfa_node_t* hook_node = hook->node;

		scf_dfa_clear_hooks(&(dfa->hooks[SCF_DFA_HOOK_POST]), hook->next);
		hook = NULL;

		scf_logi("\033[32m post hook: %s->action()\033[0m\n", hook_node->name);

		if (hook_node != node && hook_node->action) {

			ret = hook_node->action(dfa, words, data);

			if (SCF_DFA_SWITCH_TO == ret) {
				scf_logi("\033[31m post hook: switch to %s->%s\033[0m\n", node->name, hook_node->name);

				node = hook_node;
				ret = SCF_DFA_NEXT_WORD;
			}
		}
	}

	if (SCF_DFA_OK == ret) {

		scf_dfa_hook_t** pp = &(dfa->hooks[SCF_DFA_HOOK_END]);

		while (*pp) {
			scf_dfa_hook_t* hook = *pp;
			scf_dfa_node_t* hook_node = hook->node;

			*pp = hook->next;
			free(hook);
			hook = NULL;

			scf_logi("\033[32m end hook: %s->action()\033[0m\n", hook_node->name);

			if (!hook_node->action)
				continue;

			ret = hook_node->action(dfa, words, data);

			if (SCF_DFA_OK == ret)
				continue;

			if (SCF_DFA_SWITCH_TO == ret) {
				scf_logi("\033[31m end hook: switch to %s->%s\033[0m\n", node->name, hook_node->name);

				node = hook_node;
				ret  = SCF_DFA_NEXT_WORD;
			}
			break;
		}
	}

	if (SCF_DFA_NEXT_WORD == ret) {

		scf_lex_word_t* w = dfa->ops->pop_word(dfa);
		if (!w) {
			scf_loge("SCF_DFA_ERROR\n");
			return SCF_DFA_ERROR;
		}

		scf_vector_add(words, w);

		scf_logd("pop w->type: %d, '%s', line: %d, pos: %d\n", w->type, w->text->data, w->line, w->pos);

	} else if (SCF_DFA_OK == ret) {
		scf_logi("SCF_DFA_OK\n");
		return SCF_DFA_OK;

	} else if (SCF_DFA_CONTINUE == ret) {

	} else {
		scf_loge("SCF_DFA: %d\n", ret);
		return SCF_DFA_ERROR;
	}

_continue:
	if (!node->childs || node->childs->size <= 0) {
		scf_logi("SCF_DFA_NEXT_SYNTAX\n");
		return SCF_DFA_NEXT_SYNTAX;
	}

	ret = _scf_dfa_childs_parse_word(dfa, (scf_dfa_node_t**)node->childs->data, node->childs->size, words, data);
	return ret;
}

int scf_dfa_parse_word(scf_dfa_t* dfa, void* word, void* data)
{
	if (!dfa || !word) {
		scf_loge("\n");
		return -1;
	}

	if (!dfa->syntaxes || dfa->syntaxes->size <= 0) {
		scf_loge("\n");
		return -1;
	}

	if (!dfa->ops || !dfa->ops->pop_word) {
		scf_loge("\n");
		return -1;
	}

	scf_vector_t*   words = scf_vector_alloc();
	scf_lex_word_t* w     = word;

	int ret;
	int i;

//	assert(!dfa->words);
//	dfa->words = scf_vector_alloc();

	scf_vector_add(words, word);
//	scf_vector_add(dfa->words, word);

	ret = _scf_dfa_childs_parse_word(dfa, (scf_dfa_node_t**)dfa->syntaxes->data, dfa->syntaxes->size, words, data);


	if (SCF_DFA_OK != ret) {
		assert(words->size >= 1);

		w = words->data[words->size - 1];
		scf_loge("ret: %d, w->type: %d, '%s', line: %d\n\n", ret, w->type, w->text->data, w->line);

		scf_vector_free(words);
		words = NULL;
		return SCF_DFA_ERROR;
	}

	for (i = 0; i < words->size; i++) {
		scf_lex_word_t* w = words->data[i];
		scf_logd("##free w: %p, w->type: %d, '%s'\n", w, w->type, w->text->data);

		dfa->ops->free_word(words->data[i]);
		words->data[i] = NULL;
	}

	scf_vector_free(words);
	words = NULL;

	return SCF_DFA_OK;
}

