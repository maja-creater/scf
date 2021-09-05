#include"scf_parse.h"
#include"scf_x64.h"
#include"scf_operator_handler_semantic.h"
#include"scf_operator_handler_const.h"
#include"scf_dfa.h"
#include"scf_basic_block.h"
#include"scf_optimizer.h"
#include"scf_elf.h"
#include"scf_leb128.h"

#define ADD_SECTION_SYMBOL(sh_index, sh_name) \
	do { \
		int ret = _scf_parse_add_sym(parse, sh_name, 0, 0, sh_index, ELF64_ST_INFO(STB_LOCAL, STT_SECTION)); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while (0)

scf_base_type_t	base_types[] = {
	{SCF_VAR_CHAR,		"char",		1},

	{SCF_VAR_VOID,      "void",     1},

	{SCF_VAR_INT, 		"int",		4},
	{SCF_VAR_FLOAT,     "float",    4},
	{SCF_VAR_DOUBLE, 	"double",	8},

	{SCF_VAR_I8,        "int8_t",     1},
	{SCF_VAR_I16,       "int16_t",    2},
	{SCF_VAR_I32,       "int32_t",    4},
	{SCF_VAR_I64,       "int64_t",    8},

	{SCF_VAR_U8,        "uint8_t",    1},
	{SCF_VAR_U16,       "uint16_t",   2},
	{SCF_VAR_U32,       "uint32_t",   4},
	{SCF_VAR_U64,       "uint64_t",   8},

	{SCF_VAR_INTPTR,    "intptr_t",   sizeof(void*)},
	{SCF_VAR_UINTPTR,   "uintptr_t",  sizeof(void*)},
	{SCF_FUNCTION_PTR,  "funcptr",    sizeof(void*)},
};

extern scf_dfa_module_t  dfa_module_include;

extern scf_dfa_module_t  dfa_module_identity;

extern scf_dfa_module_t  dfa_module_expr;
extern scf_dfa_module_t  dfa_module_create;
extern scf_dfa_module_t  dfa_module_call;
extern scf_dfa_module_t  dfa_module_sizeof;
extern scf_dfa_module_t  dfa_module_container;
extern scf_dfa_module_t  dfa_module_init_data;
extern scf_dfa_module_t  dfa_module_va_arg;

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
extern scf_dfa_module_t  dfa_module_async;
#endif
extern scf_dfa_module_t  dfa_module_block;

scf_dfa_module_t* dfa_modules[] =
{
	&dfa_module_include,

	&dfa_module_identity,

	&dfa_module_expr,
	&dfa_module_create,
	&dfa_module_call,
	&dfa_module_sizeof,
	&dfa_module_container,
	&dfa_module_init_data,
	&dfa_module_va_arg,

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
	&dfa_module_async,
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

int	scf_parse_open(scf_parse_t** pparse)
{
	assert(pparse);

	scf_parse_t* parse = calloc(1, sizeof(scf_parse_t));
	assert(parse);

	scf_list_init(&parse->word_list_head);
	scf_list_init(&parse->error_list_head);

	scf_list_init(&parse->code_list_head);

	if (scf_ast_open(&parse->ast) < 0) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i < sizeof(base_types) / sizeof(base_types[0]); i++) {
		scf_ast_add_base_type(parse->ast, &base_types[i]);
	}

	if (scf_parse_dfa_init(parse) < 0) {
		scf_loge("\n");
		return -1;
	}

	parse->symtab = scf_vector_alloc();
	if (!parse->symtab)
		return -1;

	parse->global_consts = scf_vector_alloc();
	if (!parse->global_consts)
		goto const_error;

	parse->debug = scf_dwarf_debug_alloc();
	if (!parse->debug)
		goto debug_error;

	*pparse = parse;
	return 0;

debug_error:
	scf_vector_free(parse->global_consts);
const_error:
	scf_vector_free(parse->symtab);
	return -1;
}

static int _find_sym(const void* v0, const void* v1)
{
	const char*          name = v0;
	const scf_elf_sym_t* sym  = v1;

	if (!sym->name)
		return -1;

	return strcmp(name, sym->name);
}

static int _scf_parse_add_sym(scf_parse_t* parse, const char* name,
		uint64_t st_size, Elf64_Addr st_value,
		uint16_t st_shndx, uint8_t st_info)
{
	scf_elf_sym_t* sym  = NULL;
	scf_elf_sym_t* sym2 = NULL;

	if (name)
		sym = scf_vector_find_cmp(parse->symtab, name, _find_sym);

	if (!sym) {
		sym = calloc(1, sizeof(scf_elf_sym_t));
		if (!sym)
			return -ENOMEM;

		if (name) {
			sym->name = strdup(name);
			if (!sym->name) {
				free(sym);
				return -ENOMEM;
			}
		}

		sym->st_size  = st_size;
		sym->st_value = st_value;
		sym->st_shndx = st_shndx;
		sym->st_info  = st_info;

		int ret = scf_vector_add(parse->symtab, sym);
		if (ret < 0) {
			if (sym->name)
				free(sym->name);
			free(sym);
			scf_loge("\n");
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

int scf_parse_file(scf_parse_t* parse, const char* path)
{
	assert(parse);

	scf_block_t* root = parse->ast->root_block;
	scf_block_t* b    = NULL;

	int i;
	for (i = 0; i < root->node.nb_nodes; i++) {
		b  = (scf_block_t*)root->node.nodes[i];

		if (SCF_OP_BLOCK != b->node.type)
			continue;

		if (!strcmp(b->name->data, path))
			break;
	}

	if (i < root->node.nb_nodes)
		return 0;

	if (scf_lex_open(&parse->lex, path) < 0) {
		scf_loge("\n");
		return -1;
	}
	scf_ast_add_file_block(parse->ast, path);


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

static int _debug_abbrev_find_by_tag(const void* v0, const void* v1)
{
	const scf_dwarf_uword_t               tag = (scf_dwarf_uword_t)(uintptr_t)v0;
	const scf_dwarf_abbrev_declaration_t* d   = v1;

	return tag != d->tag;
}

static scf_dwarf_info_entry_t* _debug_find_type(scf_parse_t* parse, scf_type_t* t, int nb_pointers)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;

	scf_dwarf_uword_t               tag;
	scf_vector_t*                   types;

	if (nb_pointers > 0) {

		tag   = DW_TAG_pointer_type;
		types = parse->debug->base_types;

	} else if (t->type < SCF_STRUCT) {

		tag   = DW_TAG_base_type;
		types = parse->debug->base_types;

	} else {
		tag   = DW_TAG_structure_type;
		types = parse->debug->struct_types;
	}

	d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)(uintptr_t)tag, _debug_abbrev_find_by_tag);
	if (!d)
		return NULL;

	int i;
	int j;
	for (i = 0; i < types->size; i++) {
		ie =        types->data[i];

		if (ie->code != d->code)
			continue;

		assert(ie->attributes->size == d->attributes->size);

		if (ie->type == t->type && ie->nb_pointers == nb_pointers)
			return ie;
#if 0
		for (j = 0; j < d ->attributes->size; j++) {
			attr      = d ->attributes->data[j];

			if (DW_AT_name != attr->name)
				continue;

			iattr     = ie->attributes->data[j];

			if (!scf_string_cmp(iattr->data, t->name))
				return ie;
		}
#endif
	}

	return NULL;
}

static int __debug_add_type(scf_dwarf_info_entry_t** pie, scf_dwarf_abbrev_declaration_t** pad, scf_parse_t* parse,
		scf_type_t* t, int nb_pointers, scf_dwarf_info_entry_t* ie_type)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;
	scf_vector_t*                   types;

	int ret;

	if (nb_pointers > 0) {

		d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_pointer_type, _debug_abbrev_find_by_tag);
		if (!d) {
			ret = scf_dwarf_abbrev_add_pointer_type(parse->debug->abbrevs);
			if (ret < 0) {
				scf_loge("\n");
				return -1;
			}

			scf_loge("abbrevs->size: %d\n", parse->debug->abbrevs->size);

			d = parse->debug->abbrevs->data[parse->debug->abbrevs->size - 1];
		}

		types = parse->debug->base_types;

	} else if (t->type < SCF_STRUCT) {

		d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_base_type, _debug_abbrev_find_by_tag);
		if (!d) {
			ret = scf_dwarf_abbrev_add_base_type(parse->debug->abbrevs);
			if (ret < 0) {
				scf_loge("\n");
				return -1;
			}

			scf_loge("abbrevs->size: %d\n", parse->debug->abbrevs->size);

			d = parse->debug->abbrevs->data[parse->debug->abbrevs->size - 1];
		}

		types = parse->debug->base_types;

	} else {
		d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_structure_type, _debug_abbrev_find_by_tag);
		if (!d) {
			ret = scf_dwarf_abbrev_add_struct_type(parse->debug->abbrevs);
			if (ret < 0) {
				scf_loge("\n");
				return -1;
			}

			d = parse->debug->abbrevs->data[parse->debug->abbrevs->size - 1];
		}

		types = parse->debug->struct_types;
	}

	ie = scf_dwarf_info_entry_alloc();
	if (!ie)
		return -ENOMEM;
	ie->code = d->code;

	ie->type = t->type;
	ie->nb_pointers = nb_pointers;

	ret = scf_vector_add(types, ie);
	if (ret < 0) {
		scf_dwarf_info_entry_free(ie);
		return ret;
	}

	int j;
	for (j = 0; j < d ->attributes->size; j++) {
		attr      = d ->attributes->data[j];

		iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
		if (!iattr)
			return -ENOMEM;

		ret = scf_vector_add(ie->attributes, iattr);
		if (ret < 0) {
			free(iattr);
			return ret;
		}

		iattr->name = attr->name;
		iattr->form = attr->form;

		if (DW_AT_byte_size == iattr->name) {

			uint32_t byte_size;

			if (nb_pointers > 0)
				byte_size = sizeof(void*);
			else
				byte_size = t->size;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&byte_size, sizeof(byte_size));
			if (ret < 0)
				return ret;

		} else if (DW_AT_encoding == iattr->name) {

			uint8_t ate;

			if (SCF_VAR_CHAR == t->type || SCF_VAR_I8 == t->type)
				ate = DW_ATE_signed_char;

			else if (scf_type_is_signed(t->type))
				ate = DW_ATE_signed;

			else if (SCF_VAR_U8 == t->type)
				ate = DW_ATE_unsigned_char;

			else if (scf_type_is_unsigned(t->type))
				ate = DW_ATE_unsigned;

			else if (scf_type_is_float(t->type))
				ate = DW_ATE_float;
			else {
				scf_loge("\n");
				return -1;
			}

			ret = scf_dwarf_info_fill_attr(iattr, &ate, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_name == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, t->name->data, t->name->len);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_file == iattr->name) {

			uint8_t file = 1;

			ret = scf_dwarf_info_fill_attr(iattr, &file, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_line == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&t->w->line, sizeof(t->w->line));
			if (ret < 0)
				return ret;

		} else if (DW_AT_type == iattr->name) {

			uint32_t type = 0;

			iattr->ref_entry = ie_type;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(type));
			if (ret < 0)
				return ret;

		} else if (DW_AT_sibling == iattr->name) {

			uint32_t type = 0;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(type));
			if (ret < 0)
				return ret;

		} else if (0 == iattr->name) {
			assert(0 == iattr->form);
		} else {
			scf_loge("iattr->name: %d\n", iattr->name);
			return -1;
		}
	}

	*pie = ie;
	*pad = d;
	return 0;
}

static int __debug_add_member_var(scf_dwarf_info_entry_t** pie, scf_parse_t* parse, scf_variable_t* v, scf_dwarf_info_entry_t* ie_type)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;

	int ret;

	d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_member, _debug_abbrev_find_by_tag);
	if (!d) {
		ret = scf_dwarf_abbrev_add_member_var(parse->debug->abbrevs);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}

		d = parse->debug->abbrevs->data[parse->debug->abbrevs->size - 1];
	}

	ie = scf_dwarf_info_entry_alloc();
	if (!ie)
		return -ENOMEM;
	ie->code = d->code;

	ret = scf_vector_add(parse->debug->struct_types, ie);
	if (ret < 0) {
		scf_dwarf_info_entry_free(ie);
		return ret;
	}

	int j;
	for (j = 0; j < d ->attributes->size; j++) {
		attr      = d ->attributes->data[j];

		iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
		if (!iattr)
			return -ENOMEM;

		ret = scf_vector_add(ie->attributes, iattr);
		if (ret < 0) {
			free(iattr);
			return ret;
		}

		iattr->name = attr->name;
		iattr->form = attr->form;

		if (DW_AT_name == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, v->w->text->data, v->w->text->len);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_file == iattr->name) {

			uint8_t file = 1;

			ret = scf_dwarf_info_fill_attr(iattr, &file, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_line == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&v->w->line, sizeof(v->w->line));
			if (ret < 0)
				return ret;

		} else if (DW_AT_type == iattr->name) {

			uint32_t type = 0;

			iattr->ref_entry = ie_type;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(type));
			if (ret < 0)
				return ret;

		} else if (DW_AT_data_member_location == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&v->offset, sizeof(v->offset));
			if (ret < 0)
				return ret;

		} else if (0 == iattr->name) {
			assert(0 == iattr->form);
		} else {
			scf_loge("iattr->name: %d\n", iattr->name);
			return -1;
		}
	}

	*pie = ie;
	return 0;
}

static int _debug_add_type(scf_dwarf_info_entry_t** pie, scf_parse_t* parse, scf_type_t* t, int nb_pointers);

static int _debug_add_struct_type(scf_dwarf_info_entry_t** pie, scf_dwarf_abbrev_declaration_t** pad, scf_parse_t* parse, scf_type_t* t)
{
//	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;

	int ret;
	int i;

	scf_vector_t* ie_member_types = scf_vector_alloc();
	if (!ie_member_types)
		return -ENOMEM;

	int nb_pointers = 0;

	for (i = 0; i < t->scope->vars->size; i++) {

		scf_dwarf_info_entry_t* ie_member;
		scf_variable_t*         v_member;
		scf_type_t*             t_member;

		v_member  = t->scope->vars->data[i];
		ret = scf_ast_find_type_type(&t_member, parse->ast, v_member->type);
		if (ret < 0)
			return ret;

		ie_member = _debug_find_type(parse, t_member, v_member->nb_pointers);
		if (!ie_member) {

			if (t_member != t) {
				ret = _debug_add_type(&ie_member, parse, t_member, v_member->nb_pointers);
				if (ret < 0) {
					scf_loge("\n");
					return ret;
				}
			} else {
				assert(v_member->nb_pointers > 0);

				if (nb_pointers < v_member->nb_pointers)
					nb_pointers = v_member->nb_pointers;
			}
		}

		if (scf_vector_add(ie_member_types, ie_member) < 0)
			return -ENOMEM;
	}

	assert(ie_member_types->size == t->scope->vars->size);

	ret = __debug_add_type(&ie, pad, parse, t, 0, NULL);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_vector_t* refills = NULL;

	if (nb_pointers > 0) {
		refills = scf_vector_alloc();
		if (!refills)
			return -ENOMEM;
	}

	for (i = 0; i < t->scope->vars->size; i++) {

		scf_dwarf_info_entry_t* ie_member;
		scf_variable_t*         v_member;

		v_member  = t->scope->vars->data[i];

		ret = __debug_add_member_var(&ie_member, parse, v_member, ie_member_types->data[i]);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (nb_pointers > 0) {
			if (scf_vector_add(refills, ie_member) < 0) {
				return -ENOMEM;
			}
		}
	}

	scf_dwarf_info_entry_t* ie0;

	ie0 = scf_dwarf_info_entry_alloc();
	if (!ie0)
		return -ENOMEM;
	ie0->code = 0;

	if (scf_vector_add(parse->debug->struct_types, ie0) < 0) {
		scf_dwarf_info_entry_free(ie0);
		return -ENOMEM;
	}

	if (nb_pointers > 0) {

		scf_dwarf_info_entry_t* ie_pointer;
		scf_dwarf_info_attr_t*  iattr;

		int j;
		for (j = 1; j <= nb_pointers; j++) {

			ret = _debug_add_type(&ie_pointer, parse, t, j);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			for (i = 0; i < t->scope->vars->size; i++) {

				scf_dwarf_info_entry_t* ie_member;
				scf_variable_t*         v_member;

				v_member  = t->scope->vars->data[i];

				if (v_member->type != t->type)
					continue;

				if (v_member->nb_pointers != j)
					continue;

				ie_member = refills->data[i];

				int k;
				for (k = 0; k < ie_member->attributes->size; k++) {
					iattr     = ie_member->attributes->data[k];

					if (DW_AT_type == iattr->name) {
						assert(!iattr->ref_entry);
						iattr->ref_entry = ie_pointer;
						break;
					}
				}
			}
		}
	}

	*pie = ie;
	return 0;
}

static int _debug_add_type(scf_dwarf_info_entry_t** pie, scf_parse_t* parse, scf_type_t* t, int nb_pointers)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;

	int ret;
	int i;

	*pie = _debug_find_type(parse, t, nb_pointers);
	if (*pie)
		return 0;

	ie = _debug_find_type(parse, t, 0);
	if (!ie) {

		if (t->type < SCF_STRUCT) {

			ret = __debug_add_type(&ie, &d, parse, t, 0, NULL);
			if (ret < 0)
				return ret;

		} else {
			ret = _debug_add_struct_type(&ie, &d, parse, t);
			if (ret < 0)
				return ret;

			*pie = _debug_find_type(parse, t, nb_pointers);
			if (*pie)
				return 0;
		}
	}

	if (0 == nb_pointers) {
		*pie = ie;
		return 0;
	}

	return __debug_add_type(pie, &d, parse, t, nb_pointers, ie);
}

static int _debug_add_var(scf_parse_t* parse, scf_node_t* node)
{
	scf_variable_t* var = node->var;
	scf_type_t*     t   = NULL;

	int ret = scf_ast_find_type_type(&t, parse->ast, var->type);
	if (ret < 0)
		return ret;

	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_declaration_t* d2;
	scf_dwarf_abbrev_declaration_t* subp;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_entry_t*         ie2;
	scf_dwarf_info_attr_t*          iattr;

	int i;
	int j;

	ie = _debug_find_type(parse, t, var->nb_pointers);
	if (!ie) {

		ret = _debug_add_type(&ie, parse, t, var->nb_pointers);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_logd("ie: %p, t->name: %s\n", ie, t->name->data);

	subp = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_subprogram, _debug_abbrev_find_by_tag);
	assert(subp);

	d2   = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_variable, _debug_abbrev_find_by_tag);
	if (!d2) {
		ret = scf_dwarf_abbrev_add_var(parse->debug->abbrevs);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}

		d2 = parse->debug->abbrevs->data[parse->debug->abbrevs->size - 1];

		assert(DW_TAG_variable == d2->tag);
	}

	for (i  = parse->debug->infos->size - 1; i >= 0; i--) {
		ie2 = parse->debug->infos->data[i];

		if (ie2->code == subp->code)
			break;

		if (ie2->code != d2->code)
			continue;

		assert(ie2->attributes->size == d2->attributes->size);

		for (j = 0; j < d2 ->attributes->size; j++) {
			attr      = d2 ->attributes->data[j];

			if (DW_AT_name != attr->name)
				continue;

			iattr     = ie2->attributes->data[j];

			if (!strcmp(iattr->data->data, var->w->text->data)) {
				scf_logd("find var: %s\n", var->w->text->data);
				return 0;
			}
		}
	}

	ie2 = scf_dwarf_info_entry_alloc();
	if (!ie2)
		return -ENOMEM;
	ie2->code = d2->code;

	ret = scf_vector_add(parse->debug->infos, ie2);
	if (ret < 0) {
		scf_dwarf_info_entry_free(ie2);
		return ret;
	}

	for (j = 0; j < d2 ->attributes->size; j++) {
		attr      = d2 ->attributes->data[j];

		iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
		if (!iattr)
			return -ENOMEM;

		int ret = scf_vector_add(ie2->attributes, iattr);
		if (ret < 0) {
			free(iattr);
			return ret;
		}

		iattr->name = attr->name;
		iattr->form = attr->form;

		if (DW_AT_name == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, var->w->text->data, var->w->text->len);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_file == iattr->name) {

			uint8_t file = 1;

			ret = scf_dwarf_info_fill_attr(iattr, &file, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_line == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&var->w->line, sizeof(var->w->line));
			if (ret < 0)
				return ret;

		} else if (DW_AT_type == iattr->name) {

			uint32_t type = 0;

			iattr->ref_entry = ie;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(uint32_t));
			if (ret < 0)
				return ret;

		} else if (DW_AT_location == iattr->name) {

			if (!var->local_flag) {
				scf_loge("var: %s\n", var->w->text->data);
				return -1;
			}

			scf_logd("var->bp_offset: %d, var: %s\n", var->bp_offset, var->w->text->data);

			uint8_t buf[64];

			buf[sizeof(scf_dwarf_uword_t)] = DW_OP_fbreg;

			size_t len = scf_leb128_encode_int32(var->bp_offset,
					buf         + sizeof(scf_dwarf_uword_t) + 1,
					sizeof(buf) - sizeof(scf_dwarf_uword_t) - 1);
			assert(len > 0);

			*(scf_dwarf_uword_t*) buf = 1 + len;

			ret = scf_dwarf_info_fill_attr(iattr, buf, sizeof(scf_dwarf_uword_t) + 1 + len);
			if (ret < 0)
				return ret;

		} else if (0 == iattr->name) {
			assert(0 == iattr->form);
		} else {
			scf_loge("\n");
			return -1;
		}
	}

	return 0;
}

static int _fill_code_list_inst(scf_string_t* code, scf_list_t* h, int64_t offset, scf_parse_t* parse, scf_function_t* f)
{
	scf_list_t* l;
	scf_node_t* node;

	uint32_t    line  = 0;
	uint32_t    line2 = 0;

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

		if (c->dsts) {
			scf_3ac_operand_t* dst;

			for (i  = 0; i < c->dsts->size; i++) {
				dst =        c->dsts->data[i];

				if (!dst->dag_node || !dst->dag_node->node)
					continue;

				if (dst->debug_w && line2 < dst->debug_w->line)
					line2 = dst->debug_w->line;

				node = dst->dag_node->node;

				if (node->debug_w) {
					if (scf_type_is_var(node->type) && node->var->local_flag) {

						ret = _debug_add_var(parse, node);
						if (ret < 0)
							return ret;
					}
				}
			}
		}

		if (c->srcs) {
			scf_3ac_operand_t* src;

			for (i = 0; i < c->srcs->size; i++) {
				src       = c->srcs->data[i];

				if (!src->dag_node || !src->dag_node->node)
					continue;

				node = src->dag_node->node;

				if (src->debug_w && line2 < src->debug_w->line)
					line2 = src->debug_w->line;

				if (node->debug_w) {
					if (scf_type_is_var(node->type) && node->var->local_flag) {

						ret = _debug_add_var(parse, node);
						if (ret < 0)
							return ret;
					}
				}
			}
		}

		if (line2 > line) {
			line  = line2;

			scf_dwarf_line_result_t* r = calloc(1, sizeof(scf_dwarf_line_result_t));
			if (!r)
				return -ENOMEM;

			r->file_name = scf_string_clone(f->node.w->file);
			if (!r->file_name) {
				free(r);
				return -ENOMEM;
			}

			r->address = offset;
			r->line    = line;
			r->is_stmt = 1;

			ret = scf_vector_add(parse->debug->lines, r);
			if (ret < 0) {
				scf_string_free(r->file_name);
				free(r);
				return ret;
			}
		}

		offset += c->inst_bytes;
	}

	return 0;
}

static int _debug_add_subprogram(scf_dwarf_info_entry_t** pie, scf_parse_t* parse, scf_function_t* f, int64_t offset)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_entry_t*         ie2;
	scf_dwarf_info_attr_t*          iattr;

	d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_subprogram, _debug_abbrev_find_by_tag);
	if (!d) {
		scf_loge("\n");
		return -1;
	}

	ie = scf_dwarf_info_entry_alloc();
	if (!ie)
		return -ENOMEM;
	ie->code = d->code;

	int ret = scf_vector_add(parse->debug->infos, ie);
	if (ret < 0) {
		scf_dwarf_info_entry_free(ie);
		return ret;
	}

	int j;
	for (j = 0; j < d ->attributes->size; j++) {
		attr      = d ->attributes->data[j];

		iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
		if (!iattr)
			return -ENOMEM;

		ret = scf_vector_add(ie->attributes, iattr);
		if (ret < 0) {
			free(iattr);
			return ret;
		}

		iattr->name = attr->name;
		iattr->form = attr->form;

		if (DW_AT_external == iattr->name) {

			uint8_t value = 1;

			ret = scf_dwarf_info_fill_attr(iattr, &value, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_name == iattr->name) {

			scf_string_t* fname = f->node.w->text;

			ret = scf_dwarf_info_fill_attr(iattr, fname->data, fname->len + 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_file == iattr->name) {
			scf_loge("f->node.w->file->data: %s\n", f->node.w->file->data);

			scf_string_t* s;
			int k;
			scf_loge("parse->debug->file_names->size: %d\n", parse->debug->file_names->size);

			for (k = 0; k < parse->debug->file_names->size; k++) {
				s  =        parse->debug->file_names->data[k];

				scf_loge("s->data: %s\n", s->data);

				if (!strcmp(s->data, f->node.w->file->data))
					break;
			}
			assert(k < parse->debug->file_names->size);
			assert(k < 254);

			uint8_t file = k + 1;

			ret = scf_dwarf_info_fill_attr(iattr, &file, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_decl_line == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&(f->node.w->line), sizeof(f->node.w->line));
			if (ret < 0)
				return ret;

		} else if (DW_AT_type == iattr->name) {

			uint32_t        type = 0;
			scf_variable_t* v    = f->rets->data[0];
			scf_type_t*     t    = NULL;

			ret = scf_ast_find_type_type(&t, parse->ast, v->type);
			if (ret < 0)
				return ret;

			ie2 = _debug_find_type(parse, t, v->nb_pointers);
			if (!ie2) {

				ret = _debug_add_type(&ie2, parse, t, v->nb_pointers);
				if (ret < 0) {
					scf_loge("\n");
					return ret;
				}
			}

			iattr->ref_entry = ie2;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(type));
			if (ret < 0)
				return ret;

		} else if (DW_AT_low_pc == iattr->name
				|| DW_AT_high_pc == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&offset, sizeof(offset));
			if (ret < 0)
				return ret;

		} else if (DW_AT_frame_base == iattr->name) {

			uint8_t buf[64];

			buf[sizeof(scf_dwarf_uword_t)] = DW_OP_call_frame_cfa;

			*(scf_dwarf_uword_t*) buf = 1;

			ret = scf_dwarf_info_fill_attr(iattr, buf, sizeof(scf_dwarf_uword_t) + 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_GNU_all_call_sites == iattr->name) {

		} else if (DW_AT_sibling == iattr->name) {

			uint32_t type = 0;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&type, sizeof(type));
			if (ret < 0)
				return ret;

		} else if (0 == iattr->name) {
			assert(0 == iattr->form);
		} else {
			scf_loge("iattr->name: %d\n", iattr->name);
			return -1;
		}
	}

	*pie = ie;
	return 0;
}

static int _debug_add_cu(scf_dwarf_info_entry_t** pie, scf_parse_t* parse, scf_function_t* f, int64_t offset)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;
	scf_dwarf_info_entry_t*         ie;
	scf_dwarf_info_attr_t*          iattr;

	d = scf_vector_find_cmp(parse->debug->abbrevs, (void*)DW_TAG_compile_unit, _debug_abbrev_find_by_tag);
	if (!d) {
		scf_loge("\n");
		return -1;
	}

	ie = scf_dwarf_info_entry_alloc();
	if (!ie)
		return -ENOMEM;
	ie->code = d->code;

	int ret = scf_vector_add(parse->debug->infos, ie);
	if (ret < 0) {
		scf_dwarf_info_entry_free(ie);
		return ret;
	}

	int j;
	for (j = 0; j < d ->attributes->size; j++) {
		attr      = d ->attributes->data[j];

		iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
		if (!iattr)
			return -ENOMEM;

		ret = scf_vector_add(ie->attributes, iattr);
		if (ret < 0) {
			free(iattr);
			return ret;
		}

		iattr->name = attr->name;
		iattr->form = attr->form;

		if (DW_AT_producer == iattr->name) {

			char* producer = "GNU C11 7.4.0 -mtune=generic -march=x86-64 -g -fstack-protector-strong";

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)producer, strlen(producer));
			if (ret < 0)
				return ret;

		} else if (DW_AT_language == iattr->name) {

			uint8_t language = 12;

			ret = scf_dwarf_info_fill_attr(iattr, &language, 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_name == iattr->name) {

			scf_string_t* fname = f->node.w->file;

			ret = scf_dwarf_info_fill_attr(iattr, fname->data, fname->len + 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_comp_dir == iattr->name) {

			uint8_t  buf[4096];

			uint8_t* dir = getcwd(buf, sizeof(buf) - 1);
			assert(dir);

			ret = scf_dwarf_info_fill_attr(iattr, dir, strlen(dir) + 1);
			if (ret < 0)
				return ret;

		} else if (DW_AT_low_pc == iattr->name
				|| DW_AT_high_pc == iattr->name) {

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&offset, sizeof(offset));
			if (ret < 0)
				return ret;

		} else if (DW_AT_stmt_list == iattr->name) {

			scf_dwarf_uword_t stmt_list = 0;

			ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&stmt_list, sizeof(stmt_list));
			if (ret < 0)
				return ret;

		} else if (0 == iattr->name) {
			assert(0 == iattr->form);
		} else {
			scf_loge("iattr->name: %d, %s\n", iattr->name, scf_dwarf_find_attribute(iattr->name));
			return -1;
		}
	}

	*pie = ie;
	return 0;
}

static int _fill_function_inst(scf_string_t* code, scf_function_t* f, int64_t offset, scf_parse_t* parse)
{
	scf_list_t* l;
	int ret;
	int i;

	scf_dwarf_abbrev_declaration_t* abbrev0 = NULL;
	scf_dwarf_info_entry_t*         subp    = NULL;
	scf_dwarf_info_entry_t*         ie0     = NULL;

	ret = _debug_add_subprogram(&subp, parse, f, offset);
	if (ret < 0)
		return ret;

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

		ret = _fill_code_list_inst(code, &bb->code_list_head, offset + f->code_bytes, parse, f);
		if (ret < 0)
			return ret;

		f->code_bytes += bb->code_bytes;
	}
#if 0
	if (f->code_bytes & 0x7) {

		size_t n = 8 - (f->code_bytes & 0x7);

		ret = scf_string_fill_zero(code, n);
		if (ret < 0)
			return ret;

		f->code_bytes += n;
	}
#endif
	uint64_t high_pc_ = f->code_bytes;

#define DEBUG_UPDATE_HIGH_PC(ie, high_pc) \
	do { \
		scf_dwarf_info_attr_t*  iattr; \
		int i; \
		\
		for (i = 0; i < ie->attributes->size; i++) { \
			iattr     = ie->attributes->data[i]; \
			\
			if (DW_AT_high_pc == iattr->name) { \
				ret = scf_dwarf_info_fill_attr(iattr, (uint8_t*)&high_pc, sizeof(high_pc)); \
				if (ret < 0) \
					return ret; \
				break; \
			} \
		} \
	} while (0)

	DEBUG_UPDATE_HIGH_PC(subp, high_pc_);

#if 1
	ie0 = scf_dwarf_info_entry_alloc();
	if (!ie0)
		return -ENOMEM;
	ie0->code = 0;

	if (scf_vector_add(parse->debug->infos, ie0) < 0) {
		scf_dwarf_info_entry_free(ie0);
		return -ENOMEM;
	}
#endif
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

static int _scf_parse_add_rela(scf_vector_t* relas, scf_parse_t* parse, scf_rela_t* r, const char* name, uint16_t st_shndx)
{
	scf_elf_rela_t* rela;

	int ret;
	int i;

	for (i = 0; i < parse->symtab->size; i++) {
		scf_elf_sym_t* sym = parse->symtab->data[i];

		if (!sym->name)
			continue;

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

	scf_logd("rela: %s, offset: %ld\n", name, r->text_offset);

	rela = calloc(1, sizeof(scf_elf_rela_t));
	if (!rela)
		return -ENOMEM;

	rela->name     = (char*)name;
	rela->r_offset = r->text_offset;
	rela->r_info   = ELF64_R_INFO(i + 1, R_X86_64_PC32);
	rela->r_addend = r->addend;

	ret = scf_vector_add(relas, rela);
	if (ret < 0) {
		scf_loge("\n");
		free(rela);
		return ret;
	}

	return 0;
}

static int _fill_data(scf_parse_t* parse, scf_variable_t* v, scf_string_t* data, uint32_t shndx)
{
	char*    name;
	int      size;
	uint8_t* v_data;

	if (v->global_flag) {
		name = v->w->text->data;
	} else
		name = v->signature->data;

	scf_logd("v_%d_%d/%s, nb_dimentions: %d\n", v->w->line, v->w->pos, v->w->text->data, v->nb_dimentions);

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

	int ret;

	if (fill_size > 0) {
		ret = scf_string_fill_zero(data, fill_size);
		if (ret < 0)
			return ret;
	}
	assert(0 == (data->len & 0x7));

	v->ds_offset = data->len;

	uint64_t stb;
	if (v->static_flag)
		stb = STB_LOCAL;
	else
		stb = STB_GLOBAL;

	ret = _scf_parse_add_sym(parse, name, size, data->len, shndx, ELF64_ST_INFO(stb, STT_OBJECT));
	if (ret < 0)
		return ret;

	if (!v_data)
		ret = scf_string_fill_zero(data, size);
	else
		ret = scf_string_cat_cstr_len(data, v_data, size);

	return ret;
}

static int _scf_parse_add_data_relas(scf_parse_t* parse, scf_elf_context_t* elf)
{
	scf_elf_rela_t* rela;
	scf_ast_rela_t* r;
	scf_function_t* f;
	scf_variable_t* v;
	scf_variable_t* v2;
	scf_vector_t*   relas;
	scf_string_t*   rodata;

	int ret;
	int i;
	int j;

	for (i = 0; i < parse->ast->global_relas->size; i++) {
		r  =        parse->ast->global_relas->data[i];

		v = r->obj->base;

		if (scf_variable_const_string (v)
				|| (scf_variable_const(v) && SCF_FUNCTION_PTR != v->type)) {

			if (scf_vector_add_unique(parse->global_consts, v) < 0)
				return -ENOMEM;
		}
	}

	rodata = scf_string_alloc();
	if (!rodata)
		return -ENOMEM;

	for (i = 0; i < parse->global_consts->size; i++) {
		v  =        parse->global_consts->data[i];

		for (j = 1; j < i; j++) {

			v2 = parse->global_consts->data[j];

			if (v2 == v)
				break;

			if (v2->type != v->type || v2->size != v->size)
				continue;

			if (scf_variable_const_string(v)) {

				assert(scf_variable_const_string(v2));

				if (!scf_string_cmp(v->data.s, v2->data.s))
					break;
				continue;
			}

			if (!memcmp(&v->data, &v2->data, v->size))
				break;
		}

		if (j < i) {
			if (v2 != v)
				v2->ds_offset = v->ds_offset;
			continue;
		}

		v->global_flag = 1;

		ret = _fill_data(parse, v, rodata, SCF_SHNDX_RODATA);
		if (ret < 0) {
			scf_string_free(rodata);
			return ret;
		}
	}

	relas = scf_vector_alloc();
	if (!relas) {
		scf_string_free(rodata);
		return -ENOMEM;
	}

	for (i = 0; i < parse->ast->global_relas->size; i++) {
		r  =        parse->ast->global_relas->data[i];

		int   offset0 = scf_member_offset(r->ref);
		int   offset1 = scf_member_offset(r->obj);
		char* name;
		int   shndx;

		if (scf_variable_const(r->obj->base)) {

			if (SCF_FUNCTION_PTR == r->obj->base->type) {

				f     = r->obj->base->func_ptr;
				name  = f->node.w->text->data;
				shndx = SCF_SHNDX_TEXT;

			} else {
				name  = r->obj->base->w->text->data;
				shndx = SCF_SHNDX_RODATA;
			}
		} else if (scf_variable_const_string(r->obj->base)) {

			name  = r->obj->base->w->text->data;
			shndx = SCF_SHNDX_RODATA;
		} else {
			name  = r->obj->base->w->text->data;
			shndx = SCF_SHNDX_DATA;
		}

		for (j = 0; j < parse->symtab->size; j++) {
			scf_elf_sym_t* sym = parse->symtab->data[j];

			if (!sym->name)
				continue;

			if (!strcmp(name, sym->name))
				break;
		}

		if (j == parse->symtab->size) {
			ret = _scf_parse_add_sym(parse, name, 0, 0, 0, ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE));
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		rela = calloc(1, sizeof(scf_elf_rela_t));
		if (!rela)
			return -ENOMEM;

		rela->name     = name;
		rela->r_offset = r->ref->base->ds_offset + offset0;
		rela->r_info   = ELF64_R_INFO(j + 1, R_X86_64_64);
		rela->r_addend = offset1;

		ret = scf_vector_add(relas, rela);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}
	}

	ret = 0;

	scf_elf_section_t ro = {0};

	ro.name         = ".rodata";
	ro.sh_type      = SHT_PROGBITS;
	ro.sh_flags     = SHF_ALLOC;
	ro.sh_addralign = 8;
	ro.data         = rodata->data;
	ro.data_len     = rodata->len;
	ro.index        = SCF_SHNDX_RODATA;

	ret = scf_elf_add_section(elf, &ro);
	if (ret < 0)
		goto error;

	if (relas->size > 0) {

		scf_elf_section_t s = {0};

		s.name         = ".rela.data";
		s.sh_type      = SHT_RELA;
		s.sh_flags     = SHF_INFO_LINK;
		s.sh_addralign = 8;
		s.data         = NULL;
		s.data_len     = 0;
		s.sh_link      = 0;
		s.sh_info      = SCF_SHNDX_DATA;

		ret = scf_elf_add_rela_section(elf, &s, relas);
	}
error:
	scf_string_free (rodata);
	scf_vector_clear(relas, (void (*)(void*) ) free);
	scf_vector_free (relas);
	return ret;
}

static int _scf_parse_add_ds(scf_parse_t* parse, scf_elf_context_t* elf, scf_vector_t* global_vars)
{
	scf_variable_t* v;
	scf_string_t*   data;

	data = scf_string_alloc();
	if (!data)
		return -ENOMEM;

	int  ret = 0;
	int  i;

	for (i = 0; i < global_vars->size; i++) {
		v  =        global_vars->data[i];

		if (v->extern_flag)
			continue;

		ret = _fill_data(parse, v, data, SCF_SHNDX_DATA);
		if (ret < 0)
			goto error;
	}

	int fill_size  = ((data->len + 7) >> 3 << 3) - data->len;
	if (fill_size > 0) {
		ret = scf_string_fill_zero(data, fill_size);
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}
	}
	assert(0 == (data->len & 0x7));

	scf_elf_section_t  ds = {0};

	ds.name         = ".data";
	ds.sh_type      = SHT_PROGBITS;
	ds.sh_flags     = SHF_ALLOC | SHF_WRITE;
	ds.sh_addralign = 8;
	ds.data         = data->data;
	ds.data_len     = data->len;
	ds.index        = SCF_SHNDX_DATA;

	ret = scf_elf_add_section(elf, &ds);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

error:
	scf_string_free(data);
	return ret;
}

static int _add_debug_section(scf_elf_context_t* elf, const char* name, const scf_string_t* bin, uint32_t shndx)
{
	scf_elf_section_t s = {0};

	s.name         = (char*)name;
	s.sh_type      = SHT_PROGBITS;
	s.sh_flags     = 0;
	s.sh_addralign = 8;
	s.data         = bin->data;
	s.data_len     = bin->len;
	s.index        = shndx;

	return scf_elf_add_section(elf, &s);
}

static int _add_debug_relas(scf_vector_t* debug_relas, scf_parse_t* parse, scf_elf_context_t* elf, int sh_index, const char* sh_name)
{
	scf_elf_sym_t* sym;
	scf_vector_t*  relas;
	scf_rela_t*    r;

	int ret;
	int i;
	int j;

	relas = scf_vector_alloc();
	if (!relas)
		return -ENOMEM;

	for (i = 0; i < debug_relas->size; i++) {
		r  =        debug_relas->data[i];

		for (j  = 0; j < parse->symtab->size; j++) {
			sym =        parse->symtab->data[j];

			if (!sym->name)
				continue;

			if (!strcmp(sym->name, r->name->data))
				break;
		}

		if (j == parse->symtab->size) {
			scf_loge("\n");
			return ret;
		}

		scf_elf_rela_t* rela = calloc(1, sizeof(scf_elf_rela_t));
		if (!rela)
			return -ENOMEM;

		if (scf_vector_add(relas, rela) < 0) {
			free(rela);
			return -ENOMEM;
		}

		scf_logd("r->name: %s\n", r->name->data);

		rela->name     = r->name->data;
		rela->r_offset = r->text_offset;
		rela->r_info   = ELF64_R_INFO(j + 1, r->type);
		rela->r_addend = r->addend;
	}

	ret = 0;
#if 1
	scf_elf_section_t s = {0};

	s.name         = (char*)sh_name;
	s.sh_type      = SHT_RELA;
	s.sh_flags     = SHF_INFO_LINK;
	s.sh_addralign = 8;
	s.data         = NULL;
	s.data_len     = 0;
	s.sh_link      = 0;
	s.sh_info      = sh_index;

	ret = scf_elf_add_rela_section(elf, &s, relas);
#endif

	scf_vector_clear(relas, (void (*)(void*) ) free);
	scf_vector_free(relas);
	return ret;
}

static int _add_debug_sections(scf_parse_t* parse, scf_elf_context_t* elf)
{
	int abbrev = _add_debug_section(elf, ".debug_abbrev", parse->debug->debug_abbrev, SCF_SHNDX_DEBUG_ABBREV);
	if (abbrev < 0)
		return abbrev;

	int info = _add_debug_section(elf, ".debug_info", parse->debug->debug_info, SCF_SHNDX_DEBUG_INFO);
	if (info < 0)
		return info;

	int line = _add_debug_section(elf, ".debug_line", parse->debug->debug_line, SCF_SHNDX_DEBUG_LINE);
	if (line < 0)
		return line;

	int str = _add_debug_section(elf, ".debug_str", parse->debug->str, SCF_SHNDX_DEBUG_STR);
	if (str < 0)
		return str;

	ADD_SECTION_SYMBOL(abbrev, ".debug_abbrev");
	ADD_SECTION_SYMBOL(info,   ".debug_info");
	ADD_SECTION_SYMBOL(line,   ".debug_line");
	ADD_SECTION_SYMBOL(str,    ".debug_str");

	return 0;
}

int scf_parse_compile_functions(scf_parse_t* parse, scf_native_t* native, scf_vector_t* functions)
{
	scf_function_t* f;

	int64_t tv0 = gettime();
	int i;
	for (i = 0; i < functions->size; i++) {
		f  =        functions->data[i];

		scf_logi("i: %d, f: %p, fname: %s, f->argv->size: %d, f->node.define_flag: %d, inline_flag: %d\n",
				i, f, f->node.w->text->data, f->argv->size, f->node.define_flag, f->inline_flag);

		if (!f->node.define_flag)
			continue;

		int ret = scf_function_semantic_analysis(parse->ast, f);
		if (ret < 0)
			return ret;

		ret = scf_function_const_opt(parse->ast, f);
		if (ret < 0)
			return ret;

		scf_list_t     h;
		scf_list_init(&h);

		ret = scf_function_to_3ac(parse->ast, f, &h);
		if (ret < 0) {
			scf_list_clear(&h, scf_3ac_code_t, list, scf_3ac_code_free);
			return ret;
		}

		ret = scf_3ac_split_basic_blocks(&h, f);
		if (ret < 0) {
			scf_list_clear(&h, scf_3ac_code_t, list, scf_3ac_code_free);
			return ret;
		}

		assert(scf_list_empty(&h));
//		scf_basic_block_print_list(&f->basic_block_list_head);
	}
	int64_t tv1 = gettime();
	scf_logw("tv1 - tv0: %ld\n", tv1 - tv0);

	int ret = scf_optimize(parse->ast, functions);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	int64_t tv2 = gettime();
	scf_logw("tv2 - tv1: %ld\n", tv2 - tv1);
#if 1
	for (i = 0; i < functions->size; i++) {
		f  =        functions->data[i];

		if (!f->node.define_flag)
			continue;

		int ret = scf_native_select_inst(native, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}
#endif

	int64_t tv3 = gettime();
	scf_logw("tv3 - tv2: %ld\n", tv3 - tv2);
	return 0;
}

static int _scf_parse_add_text_relas(scf_parse_t* parse, scf_elf_context_t* elf, scf_vector_t* functions)
{
	scf_function_t* f;
	scf_vector_t*   relas;
	scf_rela_t*     r;

	relas = scf_vector_alloc();
	if (!relas)
		return -ENOMEM;

	int ret = 0;
	int i;
	for (i = 0; i < functions->size; i++) {
		f  =        functions->data[i];

		if (!f->node.define_flag)
			continue;

		int j;
		for (j = 0; j < f->text_relas->size; j++) {
			r  =        f->text_relas->data[j];

			if (scf_function_signature(r->func) < 0) {
				scf_loge("\n");
				goto error;
			}

			if (r->func->node.define_flag)
				ret = _scf_parse_add_rela(relas, parse, r, r->func->signature->data, SCF_SHNDX_TEXT);
			else
				ret = _scf_parse_add_rela(relas, parse, r, r->func->signature->data, 0);

			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		}

		for (j = 0; j < f->data_relas->size; j++) {
			r  =        f->data_relas->data[j];

			char* name;
			if (r->var->global_flag)
				name = r->var->w->text->data;
			else
				name = r->var->signature->data;

			ret = _scf_parse_add_rela(relas, parse, r, name, 2);
			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}
		}
	}

	ret = 0;

	if (relas->size > 0) {

		scf_elf_section_t s = {0};

		s.name         = ".rela.text";
		s.sh_type      = SHT_RELA;
		s.sh_flags     = SHF_INFO_LINK;
		s.sh_addralign = 8;
		s.data         = NULL;
		s.data_len     = 0;
		s.sh_link      = 0;
		s.sh_info      = SCF_SHNDX_TEXT;

		ret = scf_elf_add_rela_section(elf, &s, relas);
	}
error:
	scf_vector_clear(relas, (void (*)(void*) ) free);
	scf_vector_free (relas);
	return ret;
}

static int _sym_cmp(const void* v0, const void* v1)
{
	const scf_elf_sym_t* sym0 = *(const scf_elf_sym_t**)v0;
	const scf_elf_sym_t* sym1 = *(const scf_elf_sym_t**)v1;

	if (STB_LOCAL == ELF64_ST_BIND(sym0->st_info)) {
		if (STB_GLOBAL == ELF64_ST_BIND(sym1->st_info))
			return -1;
	} else if (STB_LOCAL == ELF64_ST_BIND(sym1->st_info))
		return 1;
	return 0;
}

static int _add_debug_file_names(scf_parse_t* parse)
{
	scf_block_t* root = parse->ast->root_block;
	scf_block_t* b    = NULL;

	int ret;
	int i;

	for (i = 0; i < root->node.nb_nodes; i++) {
		b  = (scf_block_t*)root->node.nodes[i];

		if (SCF_OP_BLOCK != b->node.type)
			continue;

		ret = _scf_parse_add_sym(parse, b->name->data, 0, 0, SHN_ABS, ELF64_ST_INFO(STB_LOCAL, STT_FILE));
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		scf_string_t* file_str = scf_string_clone(b->name);
		if (!file_str)
			return -ENOMEM;

		ret = scf_vector_add(parse->debug->file_names, file_str);
		if (ret < 0) {
			scf_string_free(file_str);
			return ret;
		}
	}

	return 0;
}

int scf_parse_compile(scf_parse_t* parse, const char* out)
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

	scf_elf_section_t  cs = {0};

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

	ret = scf_elf_open(&elf, "x64", out, "wb");
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

	int64_t tv0 = gettime();
	ret = scf_parse_compile_functions(parse, native, functions);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	int64_t tv1 = gettime();
	scf_logw("tv1 - tv0: %ld\n", tv1 - tv0);

	ret = _add_debug_file_names(parse);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	assert(parse->debug->file_names->size > 0);
	scf_string_t* file_name = parse->debug->file_names->data[0];
	const char*   path      = file_name->data;

	ADD_SECTION_SYMBOL(SCF_SHNDX_TEXT,   ".text");
	ADD_SECTION_SYMBOL(SCF_SHNDX_RODATA, ".rodata");
	ADD_SECTION_SYMBOL(SCF_SHNDX_DATA,   ".data");

	scf_dwarf_info_entry_t*  cu = NULL;
	scf_dwarf_line_result_t* r  = NULL;
	scf_dwarf_line_result_t* r2 = NULL;

	r = calloc(1, sizeof(scf_dwarf_line_result_t));
	if (!r)
		return -ENOMEM;

	r->file_name = scf_string_cstr(path);
	if (!r->file_name) {
		free(r);
		return -ENOMEM;
	}
	r->address = 0;
	r->line    = 1;
	r->is_stmt = 1;

	if (scf_vector_add(parse->debug->lines, r) < 0) {
		scf_string_free(r->file_name);
		free(r);
		return -ENOMEM;
	}

	int64_t offset = 0;
	int i;
	for (i = 0; i < functions->size; i++) {

		scf_function_t* f = functions->data[i];

		if (!f->node.define_flag)
			continue;

		scf_logd("f: %s, code_bytes: %d\n", f->node.w->text->data, f->code_bytes);

		if (!cu) {
			ret = _debug_add_cu(&cu, parse, f, offset);
			if (ret < 0)
				return ret;
		}

		ret = _fill_function_inst(code, f, offset, parse);
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}

		scf_logd("f: %s, code_bytes: %d\n", f->node.w->text->data, f->code_bytes);

		if (scf_function_signature(f) < 0) {
			ret = -ENOMEM;
			goto error;
		}

		ret = _scf_parse_add_sym(parse, f->signature->data, f->code_bytes, offset, SCF_SHNDX_TEXT, ELF64_ST_INFO(STB_GLOBAL, STT_FUNC));
		if (ret < 0) {
			ret = -ENOMEM;
			goto error;
		}

		scf_logd("f->text_relas->size: %d\n", f->text_relas->size);
		scf_logd("f->data_relas->size: %d\n", f->data_relas->size);

		scf_rela_t* r;
		int j;
		for (j = 0; j < f->text_relas->size; j++) {
			r  =        f->text_relas->data[j];

			r->text_offset = offset + r->inst_offset;

			scf_logd("rela text %s, text_offset: %#lx, offset: %ld, inst_offset: %d\n",
					r->func->node.w->text->data, r->text_offset, offset, r->inst_offset);
		}

		for (j = 0; j < f->data_relas->size; j++) {
			r  =        f->data_relas->data[j];

			if (scf_variable_const_string (r->var)
					|| (scf_variable_const(r->var) && SCF_FUNCTION_PTR != r->var->type))

				ret = scf_vector_add_unique(parse->global_consts, r->var);
			else
				ret = scf_vector_add_unique(global_vars, r->var);

			if (ret < 0) {
				scf_loge("\n");
				goto error;
			}

			r->text_offset = offset + r->inst_offset;

			scf_logd("rela data %s, text_offset: %ld, offset: %ld, inst_offset: %d\n",
					r->var->w->text->data, r->text_offset, offset, r->inst_offset);
		}

		offset += f->code_bytes;
	}

	if (cu)
		DEBUG_UPDATE_HIGH_PC(cu, offset);

	assert(parse->debug->lines->size > 0);
	r2 = parse->debug->lines->data[parse->debug->lines->size - 1];

	r = calloc(1, sizeof(scf_dwarf_line_result_t));
	if (!r)
		return -ENOMEM;

	r->file_name = scf_string_cstr(path);
	if (!r->file_name) {
		free(r);
		return -ENOMEM;
	}

	r->address = offset;
	r->line    = r2->line;
	r->is_stmt = 1;
	r->end_sequence = 1;

	if (scf_vector_add(parse->debug->lines, r) < 0) {
		scf_string_free(r->file_name);
		free(r);
		return -ENOMEM;
	}

#if 1
	scf_dwarf_abbrev_declaration_t* abbrev0 = NULL;

	abbrev0 = scf_dwarf_abbrev_declaration_alloc();
	if (!abbrev0)
		return -ENOMEM;
	abbrev0->code = 0;

	if (scf_vector_add(parse->debug->abbrevs, abbrev0) < 0) {
		scf_dwarf_abbrev_declaration_free(abbrev0);
		return -ENOMEM;
	}
#endif

	cs.name         = ".text";
	cs.sh_type      = SHT_PROGBITS;
	cs.sh_flags     = SHF_ALLOC | SHF_EXECINSTR;
	cs.sh_addralign = 1;
	cs.data         = code->data;
	cs.data_len     = code->len;
	cs.index        = SCF_SHNDX_TEXT;

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

	ret = scf_dwarf_debug_encode(parse->debug);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	ret = _add_debug_sections(parse, elf);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	qsort(parse->symtab->data, parse->symtab->size, sizeof(void*), _sym_cmp);

	ret = _scf_parse_add_data_relas(parse, elf);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	ret = _scf_parse_add_text_relas(parse, elf, functions);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}

	if (parse->debug->line_relas->size > 0) {
		ret = _add_debug_relas(parse->debug->line_relas, parse, elf, SCF_SHNDX_DEBUG_LINE, ".rela.debug_line");
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	if (parse->debug->info_relas->size > 0) {
		ret = _add_debug_relas(parse->debug->info_relas, parse, elf, SCF_SHNDX_DEBUG_INFO, ".rela.debug_info");
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	for (i = 0; i < parse->symtab->size; i++) {
		scf_elf_sym_t* sym = parse->symtab->data[i];

		ret = scf_elf_add_sym(elf, sym, ".symtab");
		if (ret < 0) {
			scf_loge("\n");
			goto error;
		}
	}

	ret = scf_elf_write_rel(elf);
	if (ret < 0) {
		scf_loge("\n");
		goto error;
	}
	ret = 0;

	int64_t tv2 = gettime();
	scf_logw("tv2 - tv1: %ld\n", tv2 - tv1);
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

