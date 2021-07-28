#ifndef SCF_DFA_H
#define SCF_DFA_H

#include"scf_vector.h"
#include"scf_list.h"

enum scf_dfa_retvals
{
	SCF_DFA_REPEATED    = -3,

	SCF_DFA_ERROR       = -1,

	SCF_DFA_OK          = 0,
	SCF_DFA_NEXT_SYNTAX = 1,
	SCF_DFA_NEXT_WORD   = 2,
	SCF_DFA_CONTINUE    = 3,
	SCF_DFA_SWITCH_TO   = 4,
};

typedef struct scf_dfa_s           scf_dfa_t;
typedef struct scf_dfa_node_s      scf_dfa_node_t;
typedef struct scf_dfa_ops_s       scf_dfa_ops_t;
typedef struct scf_dfa_module_s    scf_dfa_module_t;
typedef struct scf_dfa_hook_s      scf_dfa_hook_t;

typedef int (*scf_dfa_is_pt)(    scf_dfa_t*	dfa, void*         word);
typedef int (*scf_dfa_action_pt)(scf_dfa_t*	dfa, scf_vector_t* words, void* data);

enum scf_dfa_hook_types {

	SCF_DFA_HOOK_PRE   = 0,
	SCF_DFA_HOOK_POST,
	SCF_DFA_HOOK_END,

	SCF_DFA_HOOK_NB
};

struct scf_dfa_hook_s {

	scf_dfa_hook_t*     next;
	scf_dfa_node_t*     node;
};

struct scf_dfa_node_s
{
	char*               name;

	scf_dfa_is_pt       is;
	scf_dfa_action_pt   action;

	scf_vector_t*       childs;

	int                 refs;

	int                 module_index;
};

struct scf_dfa_s
{
	scf_vector_t*       nodes;
	scf_vector_t*       syntaxes;

//	scf_vector_t*       words;

	scf_dfa_hook_t*     hooks[SCF_DFA_HOOK_NB];

	void*               priv;

	scf_dfa_ops_t*      ops;
};

struct scf_dfa_ops_s
{
	const char*         name;

	void*               (*pop_word)(scf_dfa_t* dfa);
	int                 (*push_word)(scf_dfa_t* dfa, void* word);
	void                (*free_word)(void* word);
};

struct scf_dfa_module_s {
	const char*         name;

	int                 (*init_module)(scf_dfa_t* dfa);
	int                 (*init_syntax)(scf_dfa_t* dfa);

	int                 (*fini_module)(scf_dfa_t* dfa);

	int                 index;
};

static inline int scf_dfa_is_entry(scf_dfa_t* dfa, void* word)
{
	return 1;
}

static inline int scf_dfa_action_entry(scf_dfa_t* dfa, scf_vector_t* words, void* data)
{
	return words->size > 0 ? SCF_DFA_CONTINUE : SCF_DFA_NEXT_WORD;
}

#define SCF_DFA_MODULE_NODE(dfa, module, node, is, action) \
	{ \
		char str[256]; \
		snprintf(str, sizeof(str) - 1, "%s_%s", dfa_module_##module.name, #node); \
		scf_dfa_node_t* node = scf_dfa_node_alloc(str, is, action); \
		if (!node) { \
			printf("%s(),%d, error: \n", __func__, __LINE__); \
			return -1; \
		} \
		node->module_index = dfa_module_##module.index; \
		scf_dfa_add_node(dfa, node); \
	}

#define SCF_DFA_GET_MODULE_NODE(dfa, module, name, node) \
	scf_dfa_node_t* node = scf_dfa_find_node(dfa, #module"_"#name); \
	if (!node) { \
		printf("%s(),%d, error: \n", __func__, __LINE__); \
		return -1; \
	}

#define SCF_DFA_MODULE_ENTRY(dfa, module) \
	SCF_DFA_MODULE_NODE(dfa, module, entry, scf_dfa_is_entry, scf_dfa_action_entry)

#define SCF_DFA_PUSH_HOOK(dfa_node, type) \
	({\
		scf_dfa_node_t* dn = (dfa_node);\
		if (!dn || !dn->is) {\
			printf("%s(), %d, error: invalid dfa node\n", __func__, __LINE__);\
			return SCF_DFA_ERROR;\
		}\
		scf_dfa_hook_t* h = calloc(1, sizeof(scf_dfa_hook_t));\
		if (!h) {\
			printf("%s(), %d, error: \n", __func__, __LINE__);\
			return SCF_DFA_ERROR;\
		}\
		h->node = dn;\
		h->next = dfa->hooks[type];\
		dfa->hooks[type] = h;\
		h;\
	})

scf_dfa_node_t*         scf_dfa_node_alloc(const char* name, scf_dfa_is_pt is, scf_dfa_action_pt action);
void                    scf_dfa_node_free(scf_dfa_node_t* node);

int                     scf_dfa_open(scf_dfa_t** pdfa, const char* name, void* priv);
void                    scf_dfa_close(scf_dfa_t* dfa);

int                     scf_dfa_add_node(scf_dfa_t* dfa, scf_dfa_node_t* node);
scf_dfa_node_t*         scf_dfa_find_node(scf_dfa_t* dfa, const char* name);

int                     scf_dfa_node_add_child(scf_dfa_node_t* parent, scf_dfa_node_t* child);

int                     scf_dfa_parse_word(scf_dfa_t* dfa, void* word, void* data);

void                    scf_dfa_del_hook(        scf_dfa_hook_t** pp, scf_dfa_hook_t* sentinel);
void                    scf_dfa_del_hook_by_name(scf_dfa_hook_t** pp, const char* name);


#endif

