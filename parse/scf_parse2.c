#include"scf_parse.h"
#include"scf_x64.h"
#include"scf_operator_handler_semantic.h"
#include"scf_operator_handler_const.h"
#include"scf_dfa.h"
#include"scf_basic_block.h"
#include"scf_optimizer.h"
#include"scf_elf.h"

scf_base_type_t	base_types[] = {
	{SCF_VAR_CHAR,		"char",		1},
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
		scf_loge("\n");
		return -1;
	}

	int nb_modules  = sizeof(dfa_modules) / sizeof(dfa_modules[0]);

	parse->dfa_data = calloc(1, sizeof(dfa_parse_data_t));
	if (!parse->dfa_data) {
		scf_loge("\n");
		return -1;
	}

	parse->dfa_data->module_datas = calloc(nb_modules, sizeof(void*));
	if (!parse->dfa_data->module_datas) {
		scf_loge("\n");
		return -1;
	}

	parse->dfa_data->current_identities = scf_stack_alloc();
	if (!parse->dfa_data->current_identities) {
		scf_loge("\n");
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
			scf_loge("init module: %s\n", m->name);
			return -1;
		}
	}

	for (i = 0; i < nb_modules; i++) {

		scf_dfa_module_t* m = dfa_modules[i];

		if (!m || !m->init_syntax)
			continue;

		if (m->init_syntax(parse->dfa) < 0) {
			scf_loge("init syntax: %s\n", m->name);
			return -1;
		}
	}

	return 0;
}

static int _add_global_function(scf_ast_t* ast, const char* fname)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_function_t* f;
	scf_variable_t* v;
	scf_type_t*     t;

	t = scf_ast_find_type_type(ast, SCF_VAR_U8);

	s = scf_string_cstr("global");
	if (!s)
		return -1;

	w = scf_lex_word_alloc(s, 0, 0, SCF_LEX_WORD_ID);
	if (!s) {
		scf_string_free(s);
		return -1;
	}

	w->text = scf_string_cstr(fname);
	if (!w->text) {
		scf_string_free(s);
		scf_lex_word_free(w);
		return -1;
	}

	f = scf_function_alloc(w);
	scf_lex_word_free(w);
	w = NULL;
	if (!f) {
		scf_string_free(s);
		return -1;
	}

	// function arg
	w = scf_lex_word_alloc(s, 0, 0, SCF_LEX_WORD_ID);
	if (!w) {
		scf_string_free(s);
		scf_function_free(f);
		return -1;
	}

	w->text = scf_string_cstr("arg");
	if (!w->text) {
		scf_string_free(s);
		scf_lex_word_free(w);
		scf_function_free(f);
		return -1;
	}

	v = SCF_VAR_ALLOC_BY_TYPE(w, t, 1, 1, NULL);
	scf_lex_word_free(w);
	w = NULL;
	if (!v) {
		scf_string_free(s);
		scf_function_free(f);
		return -1;
	}

	if (scf_vector_add(f->argv, v) < 0) {
		scf_string_free(s);
		scf_function_free(f);
		scf_variable_free(v);
		return -1;
	}

	scf_scope_push_function(ast->root_block->scope, f);
	scf_node_add_child((scf_node_t*)ast->root_block, (scf_node_t*)f);
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
		scf_loge("\n");
		return -1;
	}

	if (scf_ast_open(&parse->ast) < 0) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i < sizeof(base_types) / sizeof(base_types[0]); i++) {
		scf_ast_add_base_type(parse->ast, &base_types[i]);
	}

	scf_ast_add_file_block(parse->ast, path);

#if 1
	if (_add_global_function(parse->ast, "scf_ref") < 0) {
		scf_loge("\n");
		return -1;
	}

	if (_add_global_function(parse->ast, "scf_free") < 0) {
		scf_loge("\n");
		return -1;
	}
#endif

	if (scf_parse_dfa_init(parse) < 0) {
		scf_loge("\n");
		return -1;
	}

	parse->symtab = scf_vector_alloc();
	if (!parse->symtab) {
		scf_loge("\n");
		return -1;
	}

	*pparse = parse;
	return 0;
}

static int _find_sym(const void* v0, const void* v1)
{
	const char*          name = v0;
	const scf_elf_sym_t* sym  = v1;

	return strcmp(name, sym->name);
}

static int _scf_parse_add_sym(scf_parse_t* parse, const char* name,
		uint64_t st_size, Elf64_Addr st_value,
		uint16_t st_shndx, uint8_t st_info)
{
	scf_elf_sym_t* sym = scf_vector_find_cmp(parse->symtab, name, _find_sym);
	if (!sym) {
		sym = calloc(1, sizeof(scf_elf_sym_t));
		if (!sym)
			return -ENOMEM;

		sym->name = strdup(name);
		if (!sym->name) {
			free(sym);
			return -ENOMEM;
		}

		sym->st_size  = st_size;
		sym->st_value = st_value;
		sym->st_shndx = st_shndx;
		sym->st_info  = st_info;

		int ret = scf_vector_add(parse->symtab, sym);
		if (ret < 0) {
			free(sym->name);
			free(sym);
			return ret;
		}
	}

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
			scf_loge("lex pop word failed\n");
			return -1;
		}

		if (SCF_LEX_WORD_EOF == w->type) {
			scf_logi("eof\n");
			scf_lex_push_word(parse->lex, w);
			w = NULL;
			break;
		}

		if (SCF_LEX_WORD_SPACE == w->type) {
			scf_loge("lex word space\n");
			scf_lex_word_free(w);
			w = NULL;
			continue;
		}

		if (SCF_LEX_WORD_SEMICOLON == w->type) {
			scf_lex_word_free(w);
			w = NULL;
			continue;
		}

		assert(!d->expr);

		if (scf_dfa_parse_word(parse->dfa, w, d) < 0) {
			scf_loge("dfa parse failed\n");
			return -1;
		}
	}

	return 0;
}

static int _fill_code_list_inst(scf_string_t* code, scf_list_t* h)
{
	scf_list_t* l;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (!c->instructions)
			continue;

		int i;
		int ret;

		for (i = 0; i < c->instructions->size; i++) {

			scf_instruction_t* inst = c->instructions->data[i];

			ret = scf_string_cat_cstr_len(code, inst->code, inst->len);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int _fill_function_inst(scf_string_t* code, scf_function_t* f)
{
	scf_list_t* l;
	int ret;
	int i;

	f->code_bytes = 0;

	if (f->init_insts) {

		for (i = 0; i < f->init_insts->size; i++) {

			scf_instruction_t* inst = f->init_insts->data[i];

			ret = scf_string_cat_cstr_len(code, inst->code, inst->len);
			if (ret < 0)
				return ret;

			f->code_bytes += inst->len;
		}
	}

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head);
			l = scf_list_next(l)) {

		scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

		ret = _fill_code_list_inst(code, &bb->code_list_head);
		if (ret < 0)
			return ret;

		f->code_bytes += bb->code_bytes;
	}

	return 0;
}

static int _find_function(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (SCF_FUNCTION == node->type) {

		scf_function_t* f = (scf_function_t*)node;

		return scf_vector_add(vec, f);
	}

	return 0;
}

static int _find_global_var(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	if (SCF_OP_BLOCK == node->type
			|| (node->type >= SCF_STRUCT && node->class_flag)) {

		scf_block_t* b = (scf_block_t*)node;

		if (!b->scope || !b->scope->vars)
			return 0;

		int i;
		for (i = 0; i < b->scope->vars->size; i++) {

			scf_variable_t* v = b->scope->vars->data[i];

			if (v->global_flag || v->static_flag) {

				int ret = scf_vector_add(vec, v);
				if (ret < 0)
					return ret;
			}
		}
	}

	return 0;
}

static void _scf_loops_print(scf_vector_t* loops)
{
	int i;
	int j;
	int k;

	for (i = 0; i < loops->size; i++) {
		scf_bb_group_t* loop = loops->data[i];

		printf("loop:  %p\n", loop);
		printf("entry: %p\n", loop->entry);
		printf("exit:  %p\n", loop->exit);
		printf("body: ");
		for (j = 0; j < loop->body->size; j++)
			printf("%p ", loop->body->data[j]);
		printf("\n");

		if (loop->loop_childs) {
			printf("childs: ");
			for (k = 0; k < loop->loop_childs->size; k++)
				printf("%p ", loop->loop_childs->data[k]);
			printf("\n");
		}
		if (loop->loop_parent)
			printf("parent: %p\n", loop->loop_parent);
		printf("loop_layers: %d\n\n", loop->loop_layers);
	}
}

int scf_parse_compile_function(scf_parse_t* parse, scf_native_t* native, scf_function_t* f)
{
	int ret = 0;

	scf_list_t*    l;
	scf_list_t     code_list_head;

	ret = scf_function_semantic_analysis(parse->ast, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = scf_function_const_opt(parse->ast, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_list_init(&code_list_head);
	ret = scf_function_to_3ac(parse->ast, f, &code_list_head);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	ret = scf_3ac_split_basic_blocks(&code_list_head, f);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	assert(scf_list_empty(&code_list_head));
	scf_basic_block_print_list(&f->basic_block_list_head);

#if 1
	ret = scf_optimize(parse->ast, f, &f->basic_block_list_head);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	scf_basic_block_print_list(&f->basic_block_list_head);
	_scf_loops_print(f->bb_loops);
#endif
	return 0;
	return scf_native_select_inst(native, f);

error:
	for (l = scf_list_head(&code_list_head); l != scf_list_sentinel(&code_list_head);) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		l = scf_list_next(l);

		scf_list_del(&c->list);

		scf_3ac_code_free(c);
		c = NULL;
	}

	return ret;
}

static int _scf_parse_add_rela(scf_parse_t* parse, scf_elf_context_t* elf, scf_rela_t* r, const char* name, uint16_t st_shndx)
{
	int ret;
	int i;

	for (i = 0; i < parse->symtab->size; i++) {
		scf_elf_sym_t* sym = parse->symtab->data[i];

		if (!strcmp(name, sym->name))
			break;
	}

	if (i == parse->symtab->size) {
		ret = _scf_parse_add_sym(parse, name, 0, 0, st_shndx, ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE));
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	scf_logw("rela: %s, offset: %ld\n", name, r->text_offset);

	scf_elf_rela_t rela;

	rela.name     = (char*)name;
	rela.r_offset = r->text_offset;
	rela.r_info   = ELF64_R_INFO(i + 1, R_X86_64_PC32);
	rela.r_addend = -4;
	ret = scf_elf_add_rela(elf, &rela);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return 0;
}

static int _scf_parse_add_ds(scf_parse_t* parse, scf_elf_context_t* elf, scf_vector_t* global_vars)
{
	scf_string_t* data = scf_string_alloc();
	if (!data)
		return -ENOMEM;

	char fill[8]     = {0};
	int  ret = 0;
	int  i;

	for (i = 0; i < global_vars->size; i++) {
		scf_variable_t* v = global_vars->data[i];

		char*    name;
		int      size;
		uint8_t* v_data;

		if (v->global_flag) {
			name = v->w->text->data;
		} else
			name = v->signature->data;

		if (scf_variable_const_string(v)) {
			size   = v->data.s->len + 1;
			v_data = v->data.s->data;

		} else if (v->nb_dimentions > 0) {
			size   = scf_variable_size(v);
			v_data = v->data.p;

		} else if (v->type >= SCF_STRUCT) {
			size   = v->size;
			v_data = v->data.p;
		} else {
			size   = v->size;
			v_data = (uint8_t*)&v->data;
		}

		// align 8 bytes
		int fill_size  = (data->len + 7) >> 3 << 3;
		fill_size     -= data->len;

		if (fill_size > 0) {
			ret = scf_string_cat_cstr_len(data, fill, fill_size);
			if (ret < 0) {
				ret = -ENOMEM;
				goto error;
			}
		}
		assert(0 == (data->len & 0x7));

		ret = _scf_parse_add_sym(parse, name, size, data->len, 2, ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT));
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}

		ret = scf_string_cat_cstr_len(data, v_data, size);
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}
	}

	int fill_size  = ((data->len + 7) >> 3 << 3) - data->len;
	if (fill_size > 0) {
		ret = scf_string_cat_cstr_len(data, fill, fill_size);
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}
	}
	assert(0 == (data->len & 0x7));

	scf_elf_section_t  ds;

	ds.name         = ".data";
	ds.sh_type      = SHT_PROGBITS;
	ds.sh_flags     = SHF_ALLOC | SHF_WRITE;
	ds.sh_addralign = 8;
	ds.data         = data->data;
	ds.data_len     = data->len;

	ret = scf_elf_add_section(elf, &ds);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	ret = 0;

error:
	scf_string_free(data);
	return ret;
}

int scf_parse_compile(scf_parse_t* parse, const char* path)
{
	scf_block_t* b = parse->ast->root_block;
	if (!b)
		return -EINVAL;

	int ret = 0;

	scf_vector_t*      functions   = NULL;
	scf_vector_t*      global_vars = NULL;
	scf_elf_context_t* elf         = NULL;
	scf_native_t*      native      = NULL;
	scf_string_t*      code        = NULL;

	scf_elf_section_t  cs;

	functions = scf_vector_alloc();
	if (!functions)
		return -ENOMEM;

	global_vars = scf_vector_alloc();
	if (!global_vars) {
		ret = -ENOMEM;
		goto global_vars_error;
	}

	code = scf_string_alloc();
	if (!code) {
		ret = -ENOMEM;
		goto code_error;
	}

	ret = scf_native_open(&native, "x64");
	if (ret < 0) {
		scf_loge("open native failed\n");
		goto open_native_error;
	}

	ret = scf_elf_open(&elf, "x64", "./1.elf");
	if (ret < 0) {
		scf_loge("open elf file failed\n");
		goto open_elf_error;
	}

	ret = scf_node_search_bfs((scf_node_t*)b, NULL, functions, -1, _find_function);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	ret = scf_node_search_bfs((scf_node_t*)b, NULL, global_vars, -1, _find_global_var);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	scf_logi("all functions: %d\n",   functions->size);
	scf_logi("all global_vars: %d\n", global_vars->size);

	int64_t offset  = 0;
	int i;
	for (i = 0; i < functions->size; i++) {
		scf_function_t* f = functions->data[i];

		scf_logi("i: %d, f: %p, fname: %s, f->argv->size: %d, f->node.define_flag: %d\n",
				i, f, f->node.w->text->data, f->argv->size, f->node.define_flag);

		if (!f->node.define_flag)
			continue;

		ret = scf_parse_compile_function(parse, native, f);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}

		ret = _fill_function_inst(code, f);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}

		scf_logw("f: %s, code_bytes: %d\n", f->node.w->text->data, f->code_bytes);

		scf_string_t* s = scf_function_signature(f);
		if (!s) {
			ret = -ENOMEM;
			goto error;
		}

		ret = _scf_parse_add_sym(parse, s->data, f->code_bytes, offset, 1, ELF64_ST_INFO(STB_GLOBAL, STT_FUNC));
		scf_string_free(s);
		s = NULL;
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}

		scf_logw("f->text_relas->size: %d\n", f->text_relas->size);
		scf_logw("f->data_relas->size: %d\n", f->data_relas->size);

		int j;
		for (j = 0; j < f->text_relas->size; j++) {
			scf_rela_t* r = f->text_relas->data[j];

			r->text_offset = offset + r->inst_offset;

			scf_logw("rela text %s, text_offset: %#lx, offset: %ld, inst_offset: %d\n",
					r->func->node.w->text->data, r->text_offset, offset, r->inst_offset);
		}

		for (j = 0; j < f->data_relas->size; j++) {
			scf_rela_t* r = f->data_relas->data[j];

			ret = scf_vector_add_unique(global_vars, r->var);
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}

			r->text_offset = offset + r->inst_offset;

			scf_logw("rela data %s, text_offset: %ld, offset: %ld, inst_offset: %d\n",
					r->var->w->text->data, r->text_offset, offset, r->inst_offset);
		}

		offset += f->code_bytes;
	}

	cs.name         = ".text";
	cs.sh_type      = SHT_PROGBITS;
	cs.sh_flags     = SHF_ALLOC | SHF_EXECINSTR;
	cs.sh_addralign = 1;
	cs.data         = code->data;
	cs.data_len     = code->len;
	ret = scf_elf_add_section(elf, &cs);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	ret = _scf_parse_add_ds(parse, elf, global_vars);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	for (i = 0; i < functions->size; i++) {
		scf_function_t* f = functions->data[i];

		scf_logi("i: %d, f: %p, fname: %s, f->argv->size: %d, f->node.define_flag: %d\n",
				i, f, f->node.w->text->data, f->argv->size, f->node.define_flag);

		if (!f->node.define_flag)
			continue;

		int j;
		for (j = 0; j < f->text_relas->size; j++) {
			scf_rela_t*   r = f->text_relas->data[j];

			scf_string_t* s = scf_function_signature(r->func);

			if (r->func->node.define_flag)
				ret = _scf_parse_add_rela(parse, elf, r, s->data, 1);
			else
				ret = _scf_parse_add_rela(parse, elf, r, s->data, 0);
			scf_string_free(s);
			s = NULL;
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		}

		for (j = 0; j < f->data_relas->size; j++) {
			scf_rela_t*   r = f->data_relas->data[j];

			char* name;
			if (r->var->global_flag)
				name = r->var->w->text->data;
			else
				name = r->var->signature->data;

			ret = _scf_parse_add_rela(parse, elf, r, name, 2);
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		}
	}

	for (i = 0; i < parse->symtab->size; i++) {
		scf_elf_sym_t* sym = parse->symtab->data[i];

		ret = scf_elf_add_sym(elf, sym);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}
	}

	ret = scf_elf_write_rel(elf, NULL);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	ret = 0;

	scf_logi("ok\n\n");

error:
	scf_elf_close(elf);
open_elf_error:
	scf_native_close(native);
open_native_error:
	scf_string_free(code);
code_error:
	scf_vector_free(global_vars);
global_vars_error:
	scf_vector_free(functions);
	return ret;
}

