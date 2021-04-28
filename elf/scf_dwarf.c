#include"scf_dwarf_def.h"
#include"scf_leb128.h"

static scf_dwarf_abbrev_attribute_t abbrev_cu[] =
{
	{DW_AT_producer,    DW_FORM_strp},
	{DW_AT_language,    DW_FORM_data1},
	{DW_AT_name,        DW_FORM_strp},
	{DW_AT_comp_dir,    DW_FORM_strp},
	{DW_AT_low_pc,      DW_FORM_addr},
	{DW_AT_high_pc,     DW_FORM_data8},
	{DW_AT_stmt_list,   DW_FORM_sec_offset},
	{0,                 0},
};

static scf_dwarf_abbrev_attribute_t abbrev_subprogram[] =
{
	{DW_AT_external,           DW_FORM_flag_present},
	{DW_AT_name,               DW_FORM_strp},
	{DW_AT_decl_file,          DW_FORM_data1},
	{DW_AT_decl_line,          DW_FORM_data1},
	{DW_AT_type,               DW_FORM_ref4},
	{DW_AT_low_pc,             DW_FORM_addr},
	{DW_AT_high_pc,            DW_FORM_data8},
	{DW_AT_frame_base,         DW_FORM_exprloc},
	{DW_AT_GNU_all_call_sites, DW_FORM_flag_present},
	{DW_AT_sibling,            DW_FORM_ref4},
	{0,                        0},
};

static scf_dwarf_abbrev_attribute_t abbrev_base_type[] =
{
	{DW_AT_byte_size,          DW_FORM_data1},
	{DW_AT_encoding,           DW_FORM_data1},
	{DW_AT_name,               DW_FORM_string},
	{0,                        0},
};

static scf_dwarf_abbrev_attribute_t abbrev_base_var[] =
{
	{DW_AT_name,               DW_FORM_string},
	{DW_AT_decl_file,          DW_FORM_data1},
	{DW_AT_decl_line,          DW_FORM_data1},
	{DW_AT_type,               DW_FORM_ref4},
	{DW_AT_location,           DW_FORM_exprloc},
	{0,                        0},
};

static int __abbrev_add(scf_vector_t* abbrevs)
{
	scf_dwarf_abbrev_declaration_t* parent;
	scf_dwarf_abbrev_declaration_t* d;

	if (abbrevs->size < 1)
		return -EINVAL;

	parent = abbrevs->data[abbrevs->size - 1];

	while (parent && !parent->has_children)
		parent = parent->parent;

	if (!parent)
		return -EINVAL;

	if (!parent->childs) {
		parent->childs = scf_vector_alloc();
		if (!parent->childs)
			return -ENOMEM;
	}

	d = scf_dwarf_abbrev_declaration_alloc();
	if (!d)
		return -ENOMEM;

	if (scf_vector_add(abbrevs, d) < 0) {
		free(d);
		return -ENOMEM;
	}

	if (scf_vector_add(parent->childs, d) < 0)
		return -ENOMEM;

	d->parent = parent;
	d->code   = abbrevs->size;

	return 0;
}

static int _abbrev_add(scf_vector_t* abbrevs, scf_dwarf_uword_t tag, scf_dwarf_abbrev_attribute_t* attributes, int nb_attributes)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;

	int ret = __abbrev_add(abbrevs);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	assert(abbrevs->size > 1);

	d = abbrevs->data[abbrevs->size - 1];

	d->tag    = tag;
	d->has_children = 0;

	int i;
	for (i = 0; i < nb_attributes; i++) {

		attr = malloc(sizeof(scf_dwarf_abbrev_attribute_t));
		if (!attr)
			return -ENOMEM;

		if (scf_vector_add(d->attributes, attr) < 0) {
			free(attr);
			return -ENOMEM;
		}

		attr->name = attributes[i].name;
		attr->form = attributes[i].form;
	}
	return 0;
}

int scf_dwarf_abbrev_add_base_var(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_base_var) / sizeof(abbrev_base_var[0]);

	return _abbrev_add(abbrevs, DW_TAG_variable, abbrev_base_var, n);
}

int scf_dwarf_abbrev_add_base_type(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_base_type) / sizeof(abbrev_base_type[0]);

	return _abbrev_add(abbrevs, DW_TAG_base_type, abbrev_base_type, n);
}

int scf_dwarf_abbrev_add_subprogram(scf_vector_t* abbrevs)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;

	int ret = __abbrev_add(abbrevs);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	assert(abbrevs->size > 1);

	d = abbrevs->data[abbrevs->size - 1];

	d->tag    = DW_TAG_subprogram;
	d->has_children = 1;

	int i;
	for (i = 0; i < sizeof(abbrev_subprogram) / sizeof(abbrev_subprogram[0]); i++) {

		attr = malloc(sizeof(scf_dwarf_abbrev_attribute_t));
		if (!attr)
			return -ENOMEM;

		if (scf_vector_add(d->attributes, attr) < 0) {
			free(attr);
			return -ENOMEM;
		}

		attr->name = abbrev_subprogram[i].name;
		attr->form = abbrev_subprogram[i].form;
	}
	return 0;
}

int scf_dwarf_abbrev_add_cu(scf_vector_t* abbrevs)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_declaration_t* d2;
	scf_dwarf_abbrev_attribute_t*   attr;

	d = scf_dwarf_abbrev_declaration_alloc();
	if (!d)
		return -ENOMEM;

	if (scf_vector_add(abbrevs, d) < 0) {
		free(d);
		return -ENOMEM;
	}

	d->code = abbrevs->size;
	d->tag  = DW_TAG_compile_unit;
	d->has_children = 1;

	scf_loge("abbrevs->size: %d, d->code: %u\n", abbrevs->size, d->code);

	int i;
	for (i = 0; i < sizeof(abbrev_cu) / sizeof(abbrev_cu[0]); i++) {

		attr = malloc(sizeof(scf_dwarf_abbrev_attribute_t));
		if (!attr)
			return -ENOMEM;

		if (scf_vector_add(d->attributes, attr) < 0) {
			free(attr);
			return -ENOMEM;
		}

		attr->name = abbrev_cu[i].name;
		attr->form = abbrev_cu[i].form;
	}

	return scf_dwarf_abbrev_add_subprogram(abbrevs);
}

int scf_dwarf_debug_encode(scf_dwarf_debug_t* debug, scf_vector_t* file_names)
{
	int ret = scf_dwarf_abbrev_encode(debug->abbrevs, debug->debug_abbrev);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_dwarf_info_header_t  header;

	header.length  = 0;
	header.version = 4;
	header.offset  = 0;
	header.address_size = sizeof(void*);

	ret = scf_dwarf_info_encode(debug->infos, debug->abbrevs, debug->str, debug->debug_info, &header);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_dwarf_line_machine_t* lm = scf_dwarf_line_machine_alloc();
	if (!lm)
		return -ENOMEM;

	ret = scf_dwarf_line_machine_fill(lm, file_names);
	if (ret < 0)
		return ret;

	ret = scf_dwarf_line_encode(debug, lm, debug->lines, debug->debug_line);
	scf_dwarf_line_machine_free(lm);
	if (ret < 0)
		return ret;

	return 0;
}

scf_dwarf_debug_t* scf_dwarf_debug_alloc()
{
	scf_dwarf_debug_t* debug = calloc(1, sizeof(scf_dwarf_debug_t));
	if (!debug)
		return NULL;

	debug->lines = scf_vector_alloc();
	if (!debug->lines) {
		free(debug);
		return NULL;
	}

	debug->infos = scf_vector_alloc();
	if (!debug->infos) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->abbrevs = scf_vector_alloc();
	if (!debug->abbrevs) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->str = scf_string_alloc();
	if (!debug->str) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->debug_line = scf_string_alloc();
	if (!debug->debug_line) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->debug_info = scf_string_alloc();
	if (!debug->debug_info) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->debug_abbrev = scf_string_alloc();
	if (!debug->debug_abbrev) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->line_relas = scf_vector_alloc();
	if (!debug->line_relas) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->info_relas = scf_vector_alloc();
	if (!debug->info_relas) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	if (scf_dwarf_abbrev_add_cu(debug->abbrevs) < 0) {
		scf_dwarf_debug_free(debug);

		scf_loge("\n");
		return NULL;
	}

	return debug;
}

void scf_dwarf_debug_free (scf_dwarf_debug_t* debug)
{
	if (debug) {
		if (debug->lines)
			scf_vector_free(debug->lines);

		if (debug->infos)
			scf_vector_free(debug->infos);

		if (debug->abbrevs)
			scf_vector_free(debug->abbrevs);

		if (debug->str)
			scf_string_free(debug->str);

		if (debug->debug_line)
			scf_string_free(debug->debug_line);

		if (debug->debug_info)
			scf_string_free(debug->debug_info);

		if (debug->debug_abbrev)
			scf_string_free(debug->debug_abbrev);

		if (debug->line_relas)
			scf_vector_free(debug->line_relas);

		if (debug->info_relas)
			scf_vector_free(debug->info_relas);

		free(debug);
	}
}

