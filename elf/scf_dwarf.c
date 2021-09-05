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

static scf_dwarf_abbrev_attribute_t abbrev_var[] =
{
	{DW_AT_name,               DW_FORM_string},
	{DW_AT_decl_file,          DW_FORM_data1},
	{DW_AT_decl_line,          DW_FORM_data1},
	{DW_AT_type,               DW_FORM_ref4},
	{DW_AT_location,           DW_FORM_exprloc},
	{0,                        0},
};

static scf_dwarf_abbrev_attribute_t abbrev_struct_type[] =
{
	{DW_AT_name,               DW_FORM_strp},
	{DW_AT_byte_size,          DW_FORM_data4},
	{DW_AT_decl_file,          DW_FORM_data1},
	{DW_AT_decl_line,          DW_FORM_data1},
	{DW_AT_sibling,            DW_FORM_ref4},
	{0,                        0},
};

static scf_dwarf_abbrev_attribute_t abbrev_member_var[] =
{
	{DW_AT_name,                 DW_FORM_string},
	{DW_AT_decl_file,            DW_FORM_data1},
	{DW_AT_decl_line,            DW_FORM_data1},
	{DW_AT_type,                 DW_FORM_ref4},

	{DW_AT_data_member_location, DW_FORM_data4},
	{0,                          0},
};

static scf_dwarf_abbrev_attribute_t abbrev_pointer_type[] =
{
	{DW_AT_byte_size,          DW_FORM_data1},
	{DW_AT_type,               DW_FORM_ref4},
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

static int _abbrev_add_attributes(scf_dwarf_abbrev_declaration_t* d, scf_dwarf_abbrev_attribute_t* attributes, int nb_attributes)
{
	scf_dwarf_abbrev_attribute_t* attr;

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

static int _abbrev_add(scf_vector_t* abbrevs, scf_dwarf_uword_t tag, scf_dwarf_ubyte_t has_children, scf_dwarf_abbrev_attribute_t* attributes, int nb_attributes)
{
	scf_dwarf_abbrev_declaration_t* d;

	int ret = __abbrev_add(abbrevs);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	assert(abbrevs->size > 1);

	d = abbrevs->data[abbrevs->size - 1];

	d->tag    = tag;
	d->has_children = has_children;

	return _abbrev_add_attributes(d, attributes, nb_attributes);
}

int scf_dwarf_abbrev_add_var(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_var) / sizeof(abbrev_var[0]);

	return _abbrev_add(abbrevs, DW_TAG_variable, 0, abbrev_var, n);
}

int scf_dwarf_abbrev_add_member_var(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_member_var) / sizeof(abbrev_member_var[0]);

	return _abbrev_add(abbrevs, DW_TAG_member, 0, abbrev_member_var, n);
}

int scf_dwarf_abbrev_add_base_type(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_base_type) / sizeof(abbrev_base_type[0]);

	return _abbrev_add(abbrevs, DW_TAG_base_type, 0, abbrev_base_type, n);
}

int scf_dwarf_abbrev_add_struct_type(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_struct_type) / sizeof(abbrev_struct_type[0]);

	return _abbrev_add(abbrevs, DW_TAG_structure_type, 1, abbrev_struct_type, n);
}

int scf_dwarf_abbrev_add_pointer_type(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_pointer_type) / sizeof(abbrev_pointer_type[0]);

	return _abbrev_add(abbrevs, DW_TAG_pointer_type, 0, abbrev_pointer_type, n);
}

int scf_dwarf_abbrev_add_subprogram(scf_vector_t* abbrevs)
{
	int n = sizeof(abbrev_subprogram) / sizeof(abbrev_subprogram[0]);

	return _abbrev_add(abbrevs, DW_TAG_subprogram, 1, abbrev_subprogram, n);
}

int scf_dwarf_abbrev_add_cu(scf_vector_t* abbrevs)
{
	scf_dwarf_abbrev_declaration_t* d;

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

	int n = sizeof(abbrev_cu) / sizeof(abbrev_cu[0]);

	int ret = _abbrev_add_attributes(d, abbrev_cu, n);
	if (ret < 0)
		return ret;

	return scf_dwarf_abbrev_add_subprogram(abbrevs);
}

int scf_dwarf_debug_encode(scf_dwarf_debug_t* debug)
{
	if (0 == debug->infos->size)
		return 0;

	scf_dwarf_info_entry_t* ie;
	scf_vector_t*           infos;

	int first = 1;
	int i;

	infos = scf_vector_alloc();
	if (!infos)
		return -ENOMEM;

	ie = debug->infos->data[0];
	assert(1 == ie->code);

	if (scf_vector_add(infos, ie) < 0)
		return -ENOMEM;
#if 1
	for (i = 0; i < debug->struct_types->size; i++) {
		ie =        debug->struct_types->data[i];

		if (scf_vector_add(infos, ie) < 0)
			return -ENOMEM;

		if (0 == ie->code && first) {

			int j;
			for (j = 0; j < debug->base_types->size; j++) {
				ie =        debug->base_types->data[j];

				if (scf_vector_add(infos, ie) < 0)
					return -ENOMEM;
			}

			first = 0;
		}
	}
#endif

	for (i = 1; i < debug->infos->size; i++) {
		ie =        debug->infos->data[i];

		if (scf_vector_add(infos, ie) < 0)
			return -ENOMEM;

		if (0 == ie->code && first) {

			int j;
			for (j = 0; j < debug->base_types->size; j++) {
				ie =        debug->base_types->data[j];

				if (scf_vector_add(infos, ie) < 0)
					return -ENOMEM;
			}

			first = 0;
		}
	}

	ie = scf_dwarf_info_entry_alloc();
	if (!ie)
		return -ENOMEM;
	ie->code = 0;

	if (scf_vector_add(infos, ie) < 0) {
		scf_dwarf_info_entry_free(ie);
		return -ENOMEM;
	}

	scf_vector_free(debug->infos);
	debug->infos = infos;
	infos        = NULL;

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

	ret = scf_dwarf_info_encode(debug, &header);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_dwarf_line_machine_t* lm = scf_dwarf_line_machine_alloc();
	if (!lm)
		return -ENOMEM;

	ret = scf_dwarf_line_machine_fill(lm, debug->file_names);
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

	debug->base_types = scf_vector_alloc();
	if (!debug->base_types) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

	debug->struct_types = scf_vector_alloc();
	if (!debug->struct_types) {
		scf_dwarf_debug_free(debug);
		return NULL;
	}

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

	debug->file_names = scf_vector_alloc();
	if (!debug->file_names) {
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
		if (debug->base_types)
			scf_vector_free(debug->base_types);

		if (debug->struct_types)
			scf_vector_free(debug->struct_types);

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

		if (debug->file_names)
			scf_vector_free(debug->file_names);

		free(debug);
	}
}

