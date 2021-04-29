#include"scf_dwarf_def.h"
#include"scf_leb128.h"

typedef struct {
	uint32_t    type;
	const char* name;
} abbrev_name_t;

static abbrev_name_t  dw_tags[] =
{
	{DW_TAG_array_type,             "DW_TAG_array_type"},
	{DW_TAG_class_type,             "DW_TAG_class_type"},
	{DW_TAG_entry_point,            "DW_TAG_entry_point"},
	{DW_TAG_enumeration_type,       "DW_TAG_enumeration_type"},
	{DW_TAG_formal_parameter,       "DW_TAG_formal_parameter"},
	{DW_TAG_imported_declaration,   "DW_TAG_imported_declaration"},

	{DW_TAG_label,                  "DW_TAG_label"},
	{DW_TAG_lexical_block,          "DW_TAG_lexical_block"},

	{DW_TAG_member,                 "DW_TAG_member"},

	{DW_TAG_pointer_type,           "DW_TAG_pointer_type"},
	{DW_TAG_reference_type,         "DW_TAG_reference_type"},
	{DW_TAG_compile_unit,           "DW_TAG_compile_unit"},
	{DW_TAG_string_type,            "DW_TAG_string_type"},
	{DW_TAG_structure_type,         "DW_TAG_structure_type"},

	{DW_TAG_subroutine_type,        "DW_TAG_subroutine_type"},
	{DW_TAG_typedef,                "DW_TAG_typedef"},
	{DW_TAG_union_type,             "DW_TAG_union_type"},
	{DW_TAG_unspecified_parameters, "DW_TAG_unspecified_parameters"},
	{DW_TAG_variant,                "DW_TAG_variant"},
	{DW_TAG_common_block,           "DW_TAG_common_block"},
	{DW_TAG_common_inclusion,       "DW_TAG_common_inclusion"},
	{DW_TAG_inheritance,            "DW_TAG_inheritance"},
	{DW_TAG_inlined_subroutine,     "DW_TAG_inlined_subroutine"},
	{DW_TAG_module,                 "DW_TAG_module"},
	{DW_TAG_ptr_to_member_type,     "DW_TAG_ptr_to_member_type"},

	{DW_TAG_set_type,               "DW_TAG_set_type"},
	{DW_TAG_subrange_type,          "DW_TAG_subrange_type"},
	{DW_TAG_with_stmt,              "DW_TAG_with_stmt"},
	{DW_TAG_access_declaration,     "DW_TAG_access_declaration"},
	{DW_TAG_base_type,              "DW_TAG_base_type"},
	{DW_TAG_catch_block,            "DW_TAG_catch_block"},
	{DW_TAG_const_type,             "DW_TAG_const_type"},
	{DW_TAG_constant,               "DW_TAG_constant"},
	{DW_TAG_enumerator,             "DW_TAG_enumerator"},
	{DW_TAG_file_type,              "DW_TAG_file_type"},
	{DW_TAG_friend,                 "DW_TAG_friend"},
	{DW_TAG_namelist,               "DW_TAG_namelist"},
	{DW_TAG_namelist_item,          "DW_TAG_namelist_item"},
	{DW_TAG_packed_type,            "DW_TAG_packed_type"},
	{DW_TAG_subprogram,             "DW_TAG_subprogram"},
	{DW_TAG_template_type_param,    "DW_TAG_template_type_param"},

	{DW_TAG_template_value_param,   "DW_TAG_template_value_param"},
	{DW_TAG_thrown_type,            "DW_TAG_thrown_type"},
	{DW_TAG_try_block,              "DW_TAG_try_block"},
	{DW_TAG_variant_part,           "DW_TAG_variant_part"},
	{DW_TAG_variable,               "DW_TAG_variable"},
	{DW_TAG_volatile_type,          "DW_TAG_volatile_type"},
	{DW_TAG_lo_user,                "DW_TAG_lo_user"},
	{DW_TAG_hi_user,                "DW_TAG_hi_user"},
};

static abbrev_name_t dw_attributes[] =
{
	{ DW_AT_sibling,                "DW_AT_sibling"},
	{ DW_AT_location,               "DW_AT_location"},
	{ DW_AT_name,                   "DW_AT_name"},

	{ DW_AT_ordering,               "DW_AT_ordering"},

	{ DW_AT_byte_size,              "DW_AT_byte_size"},
	{ DW_AT_bit_offset,             "DW_AT_bit_offset"},
	{ DW_AT_bit_size,               "DW_AT_bit_size"},

	{ DW_AT_stmt_list,              "DW_AT_stmt_list"},
	{ DW_AT_low_pc,                 "DW_AT_low_pc"},
	{ DW_AT_high_pc,                "DW_AT_high_pc"},
	{ DW_AT_language,               "DW_AT_language"},

	{ DW_AT_discr,                  "DW_AT_discr"},
	{ DW_AT_discr_value,            "DW_AT_discr_value"},
	{ DW_AT_visibility,             "DW_AT_visibility"},
	{ DW_AT_import,                 "DW_AT_import"},
	{ DW_AT_string_length,          "DW_AT_string_length"},
	{ DW_AT_common_reference,       "DW_AT_common_reference"},
	{ DW_AT_comp_dir,               "DW_AT_comp_dir"},
	{ DW_AT_const_value,            "DW_AT_const_value"},
	{ DW_AT_containing_type,        "DW_AT_containing_type"},
	{ DW_AT_default_value,          "DW_AT_default_value"},

	{ DW_AT_inline,                 "DW_AT_inline"},
	{ DW_AT_is_optional,            "DW_AT_is_optional"},
	{ DW_AT_lower_bound,            "DW_AT_lower_bound"},

	{ DW_AT_producer,               "DW_AT_producer"},

	{ DW_AT_prototyped,             "DW_AT_prototyped"},

	{ DW_AT_return_addr,            "DW_AT_return_addr"},
	{ DW_AT_start_scope,            "DW_AT_start_scope"},
	{ DW_AT_stride_size,            "DW_AT_stride_size"},
	{ DW_AT_upper_bound,            "DW_AT_upper_bound"},

	{ DW_AT_abstract_origin,        "DW_AT_abstract_origin"},
	{ DW_AT_accessibility,          "DW_AT_accessibility"},
	{ DW_AT_address_class,          "DW_AT_address_class"},
	{ DW_AT_artificial,             "DW_AT_artificial"},
	{ DW_AT_base_types,             "DW_AT_base_types"},
	{ DW_AT_calling_convention,     "DW_AT_calling_convention"},
	{ DW_AT_count,                  "DW_AT_count"},
	{ DW_AT_data_member_location,   "DW_AT_data_member_location"},
	{ DW_AT_decl_column,            "DW_AT_decl_column"},
	{ DW_AT_decl_file,              "DW_AT_decl_file"},
	{ DW_AT_decl_line,              "DW_AT_decl_line"},
	{ DW_AT_declaration,            "DW_AT_declaration"},
	{ DW_AT_discr_list,             "DW_AT_discr_list"},
	{ DW_AT_encoding,               "DW_AT_encoding"},
	{ DW_AT_external,               "DW_AT_external"},
	{ DW_AT_frame_base,             "DW_AT_frame_base"},
	{ DW_AT_friend,                 "DW_AT_friend"},
	{ DW_AT_identifier_case,        "DW_AT_identifier_case"},
	{ DW_AT_macro_info,             "DW_AT_macro_info"},
	{ DW_AT_namelist_item,          "DW_AT_namelist_item"},
	{ DW_AT_priority,               "DW_AT_priority"},
	{ DW_AT_segment,                "DW_AT_segment"},
	{ DW_AT_specification,          "DW_AT_specification"},
	{ DW_AT_static_link,            "DW_AT_static_link"},
	{ DW_AT_type,                   "DW_AT_type"},
	{ DW_AT_use_location,           "DW_AT_use_location"},
	{ DW_AT_variable_parameter,     "DW_AT_variable_parameter"},
	{ DW_AT_virtuality,             "DW_AT_virtuality"},
	{ DW_AT_vtable_elem_location,   "DW_AT_vtable_elem_location"},

	{ DW_AT_lo_user,                "DW_AT_lo_user"},
	{ DW_AT_GNU_all_call_sites,     "DW_AT_GNU_all_call_sites"},
	{ DW_AT_hi_user,                "DW_AT_hi_user"},
};

// dwarf forms
static abbrev_name_t dw_forms[] =
{
	{ DW_FORM_addr,                 "DW_FORM_addr"},
	{ DW_FORM_block2,               "DW_FORM_block2"},
	{ DW_FORM_block4,               "DW_FORM_block4"},
	{ DW_FORM_data2,                "DW_FORM_data2"},
	{ DW_FORM_data4,                "DW_FORM_data4"},
	{ DW_FORM_data8,                "DW_FORM_data8"},
	{ DW_FORM_string,               "DW_FORM_string"},
	{ DW_FORM_block,                "DW_FORM_block"},
	{ DW_FORM_block1,               "DW_FORM_block1"},
	{ DW_FORM_data1,                "DW_FORM_data1"},
	{ DW_FORM_flag,                 "DW_FORM_flag"},
	{ DW_FORM_sdata,                "DW_FORM_sdata"},
	{ DW_FORM_strp,                 "DW_FORM_strp"},
	{ DW_FORM_udata,                "DW_FORM_udata"},
	{ DW_FORM_ref_addr,             "DW_FORM_ref_addr"},
	{ DW_FORM_ref1,                 "DW_FORM_ref1"},
	{ DW_FORM_ref2,                 "DW_FORM_ref2"},
	{ DW_FORM_ref4,                 "DW_FORM_ref4"},
	{ DW_FORM_ref8,                 "DW_FORM_ref8"},
	{ DW_FORM_ref_udata,            "DW_FORM_ref_udata"},
	{ DW_FORM_indirect,             "DW_FORM_indirect"},
	{ DW_FORM_sec_offset,           "DW_FORM_sec_offset"},
	{ DW_FORM_exprloc,              "DW_FORM_exprloc"},
	{ DW_FORM_flag_present,         "DW_FORM_flag_present"},
	{ DW_FORM_ref_sig8,             "DW_FORM_ref_sig8"},
};

const char* scf_dwarf_find_tag(const uint32_t type)
{
	int i;
	for (i = 0; i < sizeof(dw_tags) / sizeof(dw_tags[0]); i++) {
		if (dw_tags[i].type == type)
			return dw_tags[i].name;
	}
	return NULL;
}

const char* scf_dwarf_find_attribute(const uint32_t type)
{
	int i;
	for (i = 0; i < sizeof(dw_attributes) / sizeof(dw_attributes[0]); i++) {
		if (dw_attributes[i].type == type)
			return dw_attributes[i].name;
	}
	return NULL;
}

const char* scf_dwarf_find_form(const uint32_t type)
{
	int i;
	for (i = 0; i < sizeof(dw_forms) / sizeof(dw_forms[0]); i++) {
		if (dw_forms[i].type == type)
			return dw_forms[i].name;
	}
	return NULL;
}

scf_dwarf_abbrev_declaration_t* scf_dwarf_abbrev_declaration_alloc()
{
	scf_dwarf_abbrev_declaration_t* d = calloc(1, sizeof(scf_dwarf_abbrev_declaration_t));
	if (!d)
		return NULL;

	d->attributes = scf_vector_alloc();
	if (!d->attributes) {
		free(d);
		return NULL;
	}

	return d;
}

void scf_dwarf_abbrev_declaration_free(scf_dwarf_abbrev_declaration_t* d)
{
	if (d) {
		if (d->attributes) {
			scf_vector_clear(d->attributes, (void (*)(void*)) free);
			scf_vector_free(d->attributes);
		}

		if (d->childs)
			scf_vector_free(d->childs);

		free(d);
	}
}

int scf_dwarf_abbrev_decode(scf_vector_t* abbrev_results, const char*   debug_abbrev, size_t debug_abbrev_size)
{
	if (!abbrev_results || !debug_abbrev || 0 == debug_abbrev_size)
		return -EINVAL;

	scf_dwarf_abbrev_declaration_t* d      = NULL;
	scf_dwarf_abbrev_declaration_t* parent = NULL;
	scf_dwarf_abbrev_attribute_t*   attr   = NULL;

	int    ret;
	size_t i = 0;

	while (i < debug_abbrev_size) {

		d = scf_dwarf_abbrev_declaration_alloc();
		if (!d)
			return -ENOMEM;
		d->parent = parent;

		ret = scf_vector_add(abbrev_results, d);
		if (ret < 0) {
			scf_dwarf_abbrev_declaration_free(d);
			return ret;
		}

		size_t len;

		len     = 0;
		d->code = scf_leb128_decode_uint32(debug_abbrev + i, &len);
		assert(len > 0);
		i += len;

		if (0 == d->code) {
			parent = d->parent;
			continue;
		}
		assert(i < debug_abbrev_size);

		len    = 0;
		d->tag = scf_leb128_decode_uint32(debug_abbrev + i, &len);
		assert(len > 0);
		i += len;
		assert(i < debug_abbrev_size);

		d->has_children = debug_abbrev[i++];
		assert(i < debug_abbrev_size);

		scf_loge("d: %p, d->parent: %p, d->code: %u, d->tag: %u, d->has_children: %u\n",
				d, d->parent, d->code, d->tag, d->has_children);

		do {
			attr = malloc(sizeof(scf_dwarf_abbrev_attribute_t));
			if (!attr)
				return -ENOMEM;

			len = 0;
			attr->name = scf_leb128_decode_uint32(debug_abbrev + i, &len);
			assert(len > 0);
			i += len;
			assert(i < debug_abbrev_size);

			len = 0;
			attr->form = scf_leb128_decode_uint32(debug_abbrev + i, &len);
			assert(len > 0);
			i += len;
			assert(i < debug_abbrev_size);

			scf_logd("attr: %p, attr->name: %u, attr->form: %u\n",
				attr, attr->name, attr->form);

			int ret = scf_vector_add(d->attributes, attr);
			if (ret < 0) {
				free(attr);
				return ret;
			}
		} while (0 != attr->name && 0 != attr->form);

		assert  (0 == attr->name && 0 == attr->form);

		if (parent) {
			assert(parent->childs);

			ret = scf_vector_add(parent->childs, d);
			if (ret < 0)
				return ret;
		}

		if (d->has_children) {
			d->childs = scf_vector_alloc();
			if (!d->childs)
				return -ENOMEM;

			parent = d;
			continue;
		}
	}

	return 0;
}

static int _dwarf_abbrev_encode(scf_dwarf_abbrev_declaration_t* d, scf_string_t* debug_abbrev)
{
	scf_dwarf_abbrev_declaration_t* d2;
	scf_dwarf_abbrev_attribute_t*   attr;

	d->visited_flag = 1;

#define DWARF_ABBREV_FILL(str, buf, len) \
	do { \
		int ret = scf_string_cat_cstr_len(str, (char*)buf, len); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while(0)

	if (!d->code) {
		DWARF_ABBREV_FILL(debug_abbrev, &d->code, 1);
		return 0;
	}

	uint8_t buf[64];
	size_t  len;

	len = scf_leb128_encode_uint32(d->code, buf, sizeof(buf));
	assert(len > 0);
	DWARF_ABBREV_FILL(debug_abbrev, buf, len);

	len = scf_leb128_encode_uint32(d->tag, buf, sizeof(buf));
	assert(len > 0);
	DWARF_ABBREV_FILL(debug_abbrev, buf, len);

	DWARF_ABBREV_FILL(debug_abbrev, &d->has_children, 1);


	int j;
	for (j = 0; j < d->attributes->size; j++) {
		attr      = d->attributes->data[j];

		len = scf_leb128_encode_uint32(attr->name, buf, sizeof(buf));
		assert(len > 0);
		DWARF_ABBREV_FILL(debug_abbrev, buf, len);

		len = scf_leb128_encode_uint32(attr->form, buf, sizeof(buf));
		assert(len > 0);
		DWARF_ABBREV_FILL(debug_abbrev, buf, len);
	}

	if (!d->has_children)
		return 0;

	for (j = 0; j < d->childs->size; j++) {
		d2 =        d->childs->data[j];

		int ret = _dwarf_abbrev_encode(d2, debug_abbrev);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int scf_dwarf_abbrev_encode(scf_vector_t* abbrev_results, scf_string_t* debug_abbrev)
{
	if (!abbrev_results || !debug_abbrev)
		return -EINVAL;

	scf_dwarf_abbrev_declaration_t* d      = NULL;
	scf_dwarf_abbrev_declaration_t* parent = NULL;

	int i;
	for (i = 0; i < abbrev_results->size; i++) {
		d  =        abbrev_results->data[i];

		d->visited_flag = 0;

		if (d->code)
			scf_loge("d->code: %u, d->tag: %s\n", d->code, scf_dwarf_find_tag(d->tag));
	}

	for (i = 0; i < abbrev_results->size; i++) {
		d  =        abbrev_results->data[i];

		if (d->visited_flag)
			continue;

		int ret = _dwarf_abbrev_encode(d, debug_abbrev);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static void _dwarf_abbrev_print(scf_dwarf_abbrev_declaration_t* d)
{
	scf_dwarf_abbrev_declaration_t* d2;
	scf_dwarf_abbrev_attribute_t*   attr;

	d->visited_flag = 1;

	if (!d->code)
		return;

	printf("code: %u  %s  has_children: %u\n", d->code, scf_dwarf_find_tag(d->tag), d->has_children);

	int j;

	for (j = 0; j < d->attributes->size; j++) {
		attr      = d->attributes->data[j];

		if (0 == attr->name)
			printf("%u ", attr->name);
		else {
			int len = printf("%s ", scf_dwarf_find_attribute(attr->name));
			len = 30 - len;

			char fmt[16];
			sprintf(fmt, "%%%dc", len);
			printf(fmt, ' ');
		}

		if (0 == attr->form)
			printf("%u\n", attr->form);
		else
			printf("%s\n", scf_dwarf_find_form(attr->form));
	}

	if (!d->has_children)
		return;

	if (!d->childs)
		return;

	for (j = 0; j < d->childs->size; j++) {
		d2 =        d->childs->data[j];

		_dwarf_abbrev_print(d2);
	}
}

void scf_dwarf_abbrev_print(scf_vector_t* abbrev_results)
{
	scf_dwarf_abbrev_declaration_t* d;
	scf_dwarf_abbrev_attribute_t*   attr;

	int i;
	for (i = 0; i < abbrev_results->size; i++) {
		d  =        abbrev_results->data[i];

		d->visited_flag = 0;
	}

	for (i = 0; i < abbrev_results->size; i++) {
		d  =        abbrev_results->data[i];

		if (d->visited_flag)
			continue;

		_dwarf_abbrev_print(d);
	}
}


