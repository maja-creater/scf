#include"scf_parse.h"
#include"scf_native_x64.h"
#include"scf_operator_handler_semantic.h"
#include"scf_dfa.h"

scf_base_type_t	base_types[] = {
	{SCF_VAR_CHAR,		"char",		4},
	{SCF_VAR_STRING, 	"string",	-1},

	{SCF_VAR_INT, 		"int",		4},
	{SCF_VAR_FLOAT,     "float",    4},
	{SCF_VAR_DOUBLE, 	"double",	8},
	{SCF_VAR_COMPLEX,   "complex",  2 * 4},

	{SCF_VAR_I8,        "int8",     1},
	{SCF_VAR_I16,       "int16",    2},
	{SCF_VAR_I32,       "int32",    4},
	{SCF_VAR_I64,       "int64",    8},

	{SCF_VAR_U8,        "uint8",    1},
	{SCF_VAR_U16,       "uint16",   2},
	{SCF_VAR_U32,       "uint32",   4},
	{SCF_VAR_U64,       "uint64",   8},

	{SCF_VAR_INTPTR,    "intptr",   sizeof(void*)},
	{SCF_VAR_UINTPTR,   "uintptr",  sizeof(void*)},
	{SCF_FUNCTION_PTR,  "funcptr",  sizeof(void*)},

	{SCF_VAR_VEC2,      "vec2",     2 * 4},
	{SCF_VAR_VEC3,      "vec3",     3 * 4},
	{SCF_VAR_VEC4,      "vec4",     4 * 4},

	{SCF_VAR_MAT2x2,    "mat2x2",   2 * 2 * 4},
	{SCF_VAR_MAT3x3,    "mat3x3",   3 * 3 * 4},
	{SCF_VAR_MAT4x4,    "mat4x4",   4 * 4 * 4},
};

extern scf_dfa_module_t  dfa_module_identity;

extern scf_dfa_module_t  dfa_module_expr;
extern scf_dfa_module_t  dfa_module_create;
extern scf_dfa_module_t  dfa_module_call;
extern scf_dfa_module_t  dfa_module_sizeof;
extern scf_dfa_module_t  dfa_module_init_data;

extern scf_dfa_module_t  dfa_module_struct;
extern scf_dfa_module_t  dfa_module_union;
extern scf_dfa_module_t  dfa_module_class;

extern scf_dfa_module_t  dfa_module_type;

extern scf_dfa_module_t  dfa_module_var;

extern scf_dfa_module_t  dfa_module_function;
extern scf_dfa_module_t  dfa_module_operator;

extern scf_dfa_module_t  dfa_module_if;
extern scf_dfa_module_t  dfa_module_while;
extern scf_dfa_module_t  dfa_module_for;

#if 1
extern scf_dfa_module_t  dfa_module_break;
extern scf_dfa_module_t  dfa_module_continue;
extern scf_dfa_module_t  dfa_module_return;
extern scf_dfa_module_t  dfa_module_goto;
extern scf_dfa_module_t  dfa_module_label;
extern scf_dfa_module_t  dfa_module_error;
#endif
extern scf_dfa_module_t  dfa_module_block;

scf_dfa_module_t* dfa_modules[] =
{
	&dfa_module_identity,

	&dfa_module_expr,
	&dfa_module_create,
	&dfa_module_call,
	&dfa_module_sizeof,
	&dfa_module_init_data,

	&dfa_module_struct,
	&dfa_module_union,
	&dfa_module_class,

	&dfa_module_type,

	&dfa_module_var,

	&dfa_module_function,
	&dfa_module_operator,

	&dfa_module_if,
	&dfa_module_while,
	&dfa_module_for,

#if 1
	&dfa_module_break,
	&dfa_module_continue,
	&dfa_module_goto,
	&dfa_module_return,
	&dfa_module_label,
	&dfa_module_error,
#endif
	&dfa_module_block,
};

#define SCF_CHECK_LEX_POP_WORD(w) \
	do {\
		int ret = scf_lex_pop_word(parse->lex, &w);\
		if (ret < 0) {\
			printf("%s(),%d, error: \n", __func__, __LINE__);\
			return -1;\
		}\
	} while (0)


int scf_parse_dfa_init(scf_parse_t* parse)
{
	if (scf_dfa_open(&parse->dfa, "parse", parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int nb_modules  = sizeof(dfa_modules) / sizeof(dfa_modules[0]);

	parse->dfa_data = calloc(1, sizeof(dfa_parse_data_t));
	if (!parse->dfa_data) {

		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	parse->dfa_data->module_datas = calloc(nb_modules, sizeof(void*));
	if (!parse->dfa_data->module_datas) {

		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int i;
	for (i = 0; i < nb_modules; i++) {

		scf_dfa_module_t* m = dfa_modules[i];

		if (!m)
			continue;

		m->index = i;

		if (!m->init_module)
			continue;

		if (m->init_module(parse->dfa) < 0) {
			printf("%s(),%d, error: init module: %s\n", __func__, __LINE__, m->name);
			return -1;
		}
	}

	for (i = 0; i < nb_modules; i++) {

		scf_dfa_module_t* m = dfa_modules[i];

		if (!m || !m->init_syntax)
			continue;

		if (m->init_syntax(parse->dfa) < 0) {
			printf("%s(),%d, error: init syntax: %s\n", __func__, __LINE__, m->name);
			return -1;
		}
	}

	return 0;
}

int	scf_parse_open(scf_parse_t** pparse, const char* path)
{
	assert(pparse);
	assert(path);

	scf_parse_t* parse = calloc(1, sizeof(scf_parse_t));
	assert(parse);

	scf_list_init(&parse->word_list_head);
	scf_list_init(&parse->error_list_head);

	scf_list_init(&parse->code_list_head);

	if (scf_lex_open(&parse->lex, path) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (scf_ast_open(&parse->ast) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int i;
	for (i = 0; i < sizeof(base_types) / sizeof(base_types[0]); i++) {
		scf_ast_add_base_type(parse->ast, &base_types[i]);
	}

	scf_ast_add_file_block(parse->ast, path);

	if (scf_parse_dfa_init(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	*pparse = parse;
	return 0;
}

int scf_parse_close(scf_parse_t* parse)
{
	assert(parse);

	free(parse);
	parse = NULL;
	return 0;
}

int scf_parse_parse(scf_parse_t* parse)
{
	assert(parse);
	assert(parse->lex);

	dfa_parse_data_t* d = parse->dfa_data;

	while (1) {
		scf_lex_word_t* w = NULL;

		int ret = scf_lex_pop_word(parse->lex, &w);
		if (ret < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		if (SCF_LEX_WORD_EOF == w->type) {
			printf("%s(),%d, eof\n", __func__, __LINE__);
			scf_lex_push_word(parse->lex, w);
			w = NULL;
			break;
		}

		if (SCF_LEX_WORD_SPACE == w->type) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			assert(0); // it must not come here, already drop the SPACE in lex
			return -1;
		}

		if (SCF_LEX_WORD_SEMICOLON == w->type) {
			printf("%s(),%d, semicolon\n", __func__, __LINE__);
			scf_lex_word_free(w);
			w = NULL;
			continue;
		}

		assert(!d->expr);

		if (scf_dfa_parse_word(parse->dfa, w, d) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

typedef struct {
	char*			 fname;
	int				 argc;
	scf_variable_t** argv;
} _find_function_arg_t;

static void* _find_function(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	_find_function_arg_t* farg = arg;

	scf_block_t* b = (scf_block_t*)node;

	if (SCF_OP_BLOCK == b->node.type) {
		assert(b->scope);

		scf_function_t* f = scf_scope_find_function(b->scope, farg->fname);

		//printf("%s(),%d, fname: %s, f: %p\n", __func__, __LINE__,
		//		fname, f);

		if (f && farg->argc == f->argv->size) {

		//	printf("%s(),%d, fname: %s, f: %p, argc: %d, f->argv->size: %d\n", __func__, __LINE__,
		//			fname, f, argc, f->argv->size);

			int i;
			for (i = 0; i < farg->argc; i++) {
				scf_variable_t* v = f->argv->data[i];
				if (farg->argv[i]->type != v->type) {
					break;
				}
			}

			if (farg->argc == i) {
				return (void*)f;
			}
		}
	}

	return NULL;
}

int scf_parse_run(scf_parse_t* parse, const char* fname, const int argc, const scf_variable_t** argv, scf_variable_t** pret)
{
	scf_block_t* b = parse->ast->root_block;
	if (!b) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d, fname: %s\n", __func__, __LINE__, fname);

	_find_function_arg_t arg;
	arg.fname	= (char*)fname;
	arg.argc	= argc;
	arg.argv	= (scf_variable_t**)argv;

	scf_function_t* f = (scf_function_t*) scf_node_search_bfs((scf_node_t*)b, _find_function, &arg, NULL);
	if (!f) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("%s(),%d, \n\n", __func__, __LINE__);

	if (scf_function_semantic_analysis(parse->ast, f) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

#if 1
	printf("%s(),%d, \n\n", __func__, __LINE__);
	if (scf_function_to_3ac(parse->ast, f, &parse->code_list_head) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
#endif

	printf("%s(),%d, run function ok: f: %p, fname: %s\n\n", __func__, __LINE__, f, fname);

#if 1
	scf_3ac_context_t* _3ac_ctx = scf_3ac_context_alloc();
	scf_3ac_filter(_3ac_ctx, NULL, &parse->code_list_head);
#endif

	printf("%s(),%d, \n\n", __func__, __LINE__);
#if 0
	scf_native_context_t* native_ctx = NULL;
	if (scf_native_open(&native_ctx, "x64") < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int ret = scf_native_select_instruction(native_ctx, &parse->code_list_head, f);
	if (ret < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
#endif
	printf("%s(),%d, \n\n", __func__, __LINE__);
	return 0;
}

