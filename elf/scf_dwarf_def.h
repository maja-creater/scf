#ifndef SCF_DWARF_DEF_H
#define SCF_DWARF_DEF_H

#include"scf_list.h"
#include"scf_vector.h"
#include"scf_string.h"

typedef int8_t    scf_dwarf_sbyte_t;
typedef uint8_t   scf_dwarf_ubyte_t;
typedef uint16_t  scf_dwarf_uhalf_t;
typedef int32_t   scf_dwarf_sword_t;
typedef uint32_t  scf_dwarf_uword_t;

typedef struct    scf_dwarf_line_machine_s        scf_dwarf_line_machine_t;
typedef struct    scf_dwarf_line_prologue_s       scf_dwarf_line_prologue_t;
typedef struct    scf_dwarf_line_filename_s       scf_dwarf_line_filename_t;
typedef struct    scf_dwarf_line_result_s         scf_dwarf_line_result_t;

typedef struct    scf_dwarf_abbrev_declaration_s  scf_dwarf_abbrev_declaration_t;
typedef struct    scf_dwarf_abbrev_attribute_s    scf_dwarf_abbrev_attribute_t;

typedef struct    scf_dwarf_info_header_s         scf_dwarf_info_header_t;
typedef struct    scf_dwarf_info_entry_s          scf_dwarf_info_entry_t;
typedef struct    scf_dwarf_info_attr_s           scf_dwarf_info_attr_t;
typedef struct    scf_dwarf_attr_block_s          scf_dwarf_attr_block_t;
typedef struct    scf_dwarf_attr_ref_s            scf_dwarf_attr_ref_t;

typedef struct    scf_dwarf_debug_s               scf_dwarf_debug_t;

// dwarf line standard opcodes
#define DW_LNS_copy              1
#define DW_LNS_advance_pc        2
#define DW_LNS_advance_line      3
#define DW_LNS_set_file          4
#define DW_LNS_set_column        5
#define DW_LNS_negate_stmt       6
#define DW_LNS_set_basic_block   7
#define DW_LNS_const_add_pc      8
#define DW_LNS_fixed_advance_pc  9

// dwarf line extended opcodes
#define DW_LNE_end_sequence      1
#define DW_LNE_set_address       2
#define DW_LNE_define_file       3

// dwarf tags
#define DW_TAG_array_type             0x01
#define DW_TAG_class_type             0x02
#define DW_TAG_entry_point            0x03
#define DW_TAG_enumeration_type       0x04
#define DW_TAG_formal_parameter       0x05

#define DW_TAG_imported_declaration   0x08

#define DW_TAG_label                  0x0a
#define DW_TAG_lexical_block          0x0b

#define DW_TAG_member                 0x0d

#define DW_TAG_pointer_type           0x0f

#define DW_TAG_reference_type         0x10
#define DW_TAG_compile_unit           0x11
#define DW_TAG_string_type            0x12
#define DW_TAG_structure_type         0x13

#define DW_TAG_subroutine_type        0x15
#define DW_TAG_typedef                0x16
#define DW_TAG_union_type             0x17
#define DW_TAG_unspecified_parameters 0x18
#define DW_TAG_variant                0x19
#define DW_TAG_common_block           0x1a
#define DW_TAG_common_inclusion       0x1b
#define DW_TAG_inheritance            0x1c
#define DW_TAG_inlined_subroutine     0x1d
#define DW_TAG_module                 0x1e
#define DW_TAG_ptr_to_member_type     0x1f

#define DW_TAG_set_type               0x20
#define DW_TAG_subrange_type          0x21
#define DW_TAG_with_stmt              0x22
#define DW_TAG_access_declaration     0x23
#define DW_TAG_base_type              0x24
#define DW_TAG_catch_block            0x25
#define DW_TAG_const_type             0x26
#define DW_TAG_constant               0x27
#define DW_TAG_enumerator             0x28
#define DW_TAG_file_type              0x29
#define DW_TAG_friend                 0x2a
#define DW_TAG_namelist               0x2b
#define DW_TAG_namelist_item          0x2c
#define DW_TAG_packed_type            0x2d
#define DW_TAG_subprogram             0x2e
#define DW_TAG_template_type_param    0x2f

#define DW_TAG_template_value_param   0x30
#define DW_TAG_thrown_type            0x31
#define DW_TAG_try_block              0x32
#define DW_TAG_variant_part           0x33
#define DW_TAG_variable               0x34
#define DW_TAG_volatile_type          0x35
#define DW_TAG_dwarf_procedure        0x36
#define DW_TAG_restrict_type          0x37
#define DW_TAG_interface_type         0x38
#define DW_TAG_namespace              0x39
#define DW_TAG_imported_module        0x3a
#define DW_TAG_unspecified_type       0x3b
#define DW_TAG_partial_unit           0x3c
#define DW_TAG_imported_unit          0x3d

#define DW_TAG_condition              0x3f
#define DW_TAG_shared_type            0x40
#define DW_TAG_type_unit              0x41
#define DW_TAG_rvalue_reference_type  0x42
#define DW_TAG_template_alias         0x43
#define DW_TAG_lo_user                0x4080
#define DW_TAG_hi_user                0xffff

#define DW_CHILDREN_no                0
#define DW_CHILDREN_yes               1

// dwarf attributes
#define DW_AT_sibling                 0x01
#define DW_AT_location                0x02
#define DW_AT_name                    0x03

#define DW_AT_ordering                0x09

#define DW_AT_byte_size               0x0b
#define DW_AT_bit_offset              0x0c
#define DW_AT_bit_size                0x0d

#define DW_AT_stmt_list               0x10
#define DW_AT_low_pc                  0x11
#define DW_AT_high_pc                 0x12
#define DW_AT_language                0x13

#define DW_AT_discr                   0x15
#define DW_AT_discr_value             0x16
#define DW_AT_visibility              0x17
#define DW_AT_import                  0x18
#define DW_AT_string_length           0x19
#define DW_AT_common_reference        0x1a
#define DW_AT_comp_dir                0x1b
#define DW_AT_const_value             0x1c
#define DW_AT_containing_type         0x1d
#define DW_AT_default_value           0x1e

#define DW_AT_inline                  0x20
#define DW_AT_is_optional             0x21
#define DW_AT_lower_bound             0x22

#define DW_AT_producer                0x25

#define DW_AT_prototyped              0x27

#define DW_AT_return_addr             0x2a
#define DW_AT_start_scope             0x2c
#define DW_AT_stride_size             0x2e
#define DW_AT_upper_bound             0x2f

#define DW_AT_abstract_origin         0x31
#define DW_AT_accessibility           0x32
#define DW_AT_address_class           0x33
#define DW_AT_artificial              0x34
#define DW_AT_base_types              0x35
#define DW_AT_calling_convention      0x36
#define DW_AT_count                   0x37
#define DW_AT_data_member_location    0x38
#define DW_AT_decl_column             0x39
#define DW_AT_decl_file               0x3a
#define DW_AT_decl_line               0x3b
#define DW_AT_declaration             0x3c
#define DW_AT_discr_list              0x3d
#define DW_AT_encoding                0x3e
#define DW_AT_external                0x3f
#define DW_AT_frame_base              0x40
#define DW_AT_friend                  0x41
#define DW_AT_identifier_case         0x42
#define DW_AT_macro_info              0x43
#define DW_AT_namelist_item           0x44
#define DW_AT_priority                0x45
#define DW_AT_segment                 0x46
#define DW_AT_specification           0x47
#define DW_AT_static_link             0x48
#define DW_AT_type                    0x49
#define DW_AT_use_location            0x4a
#define DW_AT_variable_parameter      0x4b
#define DW_AT_virtuality              0x4c
#define DW_AT_vtable_elem_location    0x4d

#define DW_AT_lo_user                 0x2000
#define DW_AT_GNU_all_call_sites      0x2117
#define DW_AT_hi_user                 0x3fff

#define DW_ATE_address                0x1
#define DW_ATE_boolean                0x2
#define DW_ATE_complex_float          0x3
#define DW_ATE_float                  0x4
#define DW_ATE_signed                 0x5
#define DW_ATE_signed_char            0x6
#define DW_ATE_unsigned               0x7
#define DW_ATE_unsigned_char          0x8
#define DW_ATE_lo_user                0x80
#define DW_ATE_hi_user                0xff

// dwarf forms
#define DW_FORM_addr                  0x01
#define DW_FORM_block2                0x03
#define DW_FORM_block4                0x04
#define DW_FORM_data2                 0x05
#define DW_FORM_data4                 0x06
#define DW_FORM_data8                 0x07
#define DW_FORM_string                0x08
#define DW_FORM_block                 0x09
#define DW_FORM_block1                0x0a
#define DW_FORM_data1                 0x0b
#define DW_FORM_flag                  0x0c
#define DW_FORM_sdata                 0x0d
#define DW_FORM_strp                  0x0e
#define DW_FORM_udata                 0x0f
#define DW_FORM_ref_addr              0x10
#define DW_FORM_ref1                  0x11
#define DW_FORM_ref2                  0x12
#define DW_FORM_ref4                  0x13
#define DW_FORM_ref8                  0x14
#define DW_FORM_ref_udata             0x15
#define DW_FORM_indirect              0x16
#define DW_FORM_sec_offset            0x17
#define DW_FORM_exprloc               0x18
#define DW_FORM_flag_present          0x19
#define DW_FORM_ref_sig8              0x20

#define DW_OP_fbreg                   0x91
#define DW_OP_call_frame_cfa          0x9c

struct scf_dwarf_info_header_s
{
	scf_dwarf_uword_t  length;
	scf_dwarf_uhalf_t  version;
	scf_dwarf_uword_t  offset;
	scf_dwarf_ubyte_t  address_size;
};

struct scf_dwarf_info_entry_s
{
	scf_dwarf_uword_t  code;

	uint64_t           cu_byte_offset;

	scf_vector_t*      attributes;

	int                type;
	int                nb_pointers;
};

struct scf_dwarf_attr_ref_s
{
	scf_dwarf_info_attr_t*  iattr;

	scf_dwarf_info_entry_t* ie;
	scf_dwarf_info_entry_t* ref_entry;

	uint64_t                offset;
	size_t                  size;
};

struct scf_dwarf_info_attr_s
{
	scf_dwarf_uword_t       name;
	scf_dwarf_uword_t       form;

	scf_dwarf_info_entry_t* ref_entry;

	uintptr_t               block_ref;
	uint64_t                block_ref8;

	union {
		scf_dwarf_uword_t   block_length;

		uintptr_t           address;
		uint8_t             const1;
		uint16_t            const2;
		uint32_t            const4;
		uint64_t            const8;
		scf_dwarf_sword_t   sdata;
		scf_dwarf_uword_t   udata;

		scf_dwarf_ubyte_t   flag;

		scf_dwarf_uword_t   lineptr;
		scf_dwarf_uword_t   exprloc;

		uintptr_t           ref;
		uint64_t            ref8;

		scf_dwarf_uword_t   str_offset;
	};

	scf_string_t*           data;
};

struct scf_dwarf_abbrev_attribute_s
{
	scf_dwarf_uword_t  name;
	scf_dwarf_uword_t  form;
};

struct scf_dwarf_abbrev_declaration_s
{
	scf_dwarf_uword_t  code;
	scf_dwarf_uword_t  tag;
	scf_dwarf_ubyte_t  has_children;

	scf_vector_t*      attributes;

	scf_vector_t*      childs;

	scf_dwarf_abbrev_declaration_t* parent;

	uint32_t           visited_flag:1;
};

struct scf_dwarf_line_result_s
{
	uintptr_t          address;

	scf_string_t*      file_name;
	scf_dwarf_uword_t  line;
	scf_dwarf_uword_t  column;

	scf_dwarf_ubyte_t  is_stmt     :1;
	scf_dwarf_ubyte_t  basic_block :1;
	scf_dwarf_ubyte_t  end_sequence:1;
};

struct scf_dwarf_line_filename_s
{
	scf_dwarf_ubyte_t* name;
	scf_dwarf_uword_t  dir_index;
	scf_dwarf_uword_t  time_modified;
	scf_dwarf_uword_t  length;
};

struct scf_dwarf_line_prologue_s
{
	scf_dwarf_uword_t      total_length;
	scf_dwarf_uhalf_t      version;
	scf_dwarf_uword_t      prologue_length;

	scf_dwarf_ubyte_t      minimum_instruction_length;

	scf_dwarf_ubyte_t      default_is_stmt;

	scf_dwarf_sbyte_t      line_base;
	scf_dwarf_ubyte_t      line_range;

	scf_dwarf_ubyte_t      opcode_base;
	scf_dwarf_ubyte_t*     standard_opcode_lengths;
    scf_dwarf_ubyte_t*     include_directories;
    scf_vector_t*          file_names;
};

struct scf_dwarf_line_machine_s
{
	// registers
	uintptr_t           address;

	scf_dwarf_uword_t   file;
	scf_dwarf_uword_t   line;
	scf_dwarf_uword_t   column;

	scf_dwarf_ubyte_t   is_stmt     :1;
	scf_dwarf_ubyte_t   basic_block :1;
	scf_dwarf_ubyte_t   end_sequence:1;

	scf_dwarf_line_prologue_t* prologue;
};

struct scf_dwarf_debug_s
{
	scf_vector_t*       base_types;
	scf_vector_t*       struct_types;

	scf_vector_t*       lines;
	scf_vector_t*       infos;
	scf_vector_t*       abbrevs;
	scf_string_t*       str;

	scf_string_t*       debug_line;
	scf_string_t*       debug_info;
	scf_string_t*       debug_abbrev;

	scf_vector_t*       line_relas;
	scf_vector_t*       info_relas;
	scf_vector_t*       file_names;
};

scf_dwarf_debug_t*               scf_dwarf_debug_alloc();
void                             scf_dwarf_debug_free  (scf_dwarf_debug_t* debug);
int                              scf_dwarf_debug_encode(scf_dwarf_debug_t* debug);

int scf_dwarf_abbrev_add_cu (scf_vector_t* abbrevs);
int scf_dwarf_abbrev_add_var(scf_vector_t* abbrevs);

int scf_dwarf_abbrev_add_subprogram  (scf_vector_t* abbrevs);
int scf_dwarf_abbrev_add_struct_type (scf_vector_t* abbrevs);
int scf_dwarf_abbrev_add_member_var  (scf_vector_t* abbrevs);
int scf_dwarf_abbrev_add_base_type   (scf_vector_t* abbrevs);
int scf_dwarf_abbrev_add_pointer_type(scf_vector_t* abbrevs);

scf_dwarf_info_entry_t*          scf_dwarf_info_entry_alloc();
void                             scf_dwarf_info_entry_free(scf_dwarf_info_entry_t* ie);
void                             scf_dwarf_info_attr_free(scf_dwarf_info_attr_t* attr);

int scf_dwarf_info_fill_attr(scf_dwarf_info_attr_t* iattr, uint8_t* data, size_t len);

scf_dwarf_line_machine_t*        scf_dwarf_line_machine_alloc();
void                             scf_dwarf_line_machine_print(scf_dwarf_line_machine_t* lm);
int                              scf_dwarf_line_machine_fill(scf_dwarf_line_machine_t* lm, scf_vector_t* file_names);
void                             scf_dwarf_line_machine_free (scf_dwarf_line_machine_t* lm);
void                             scf_dwarf_line_filename_free(scf_dwarf_line_filename_t* f);

scf_dwarf_abbrev_declaration_t*  scf_dwarf_abbrev_declaration_alloc();
void                             scf_dwarf_abbrev_declaration_free(scf_dwarf_abbrev_declaration_t* d);

int scf_dwarf_line_decode(scf_dwarf_line_machine_t* lm, scf_vector_t* line_results, const char*   debug_line, size_t debug_line_size);
int scf_dwarf_line_encode(scf_dwarf_debug_t* debug, scf_dwarf_line_machine_t* lm, scf_vector_t* line_results, scf_string_t* debug_line);

int scf_dwarf_abbrev_decode(scf_vector_t* abbrev_results, const char*   debug_abbrev, size_t debug_abbrev_size);
int scf_dwarf_abbrev_encode(scf_vector_t* abbrev_results, scf_string_t* debug_abbrev);

int scf_dwarf_info_decode(scf_vector_t* infos, scf_vector_t* abbrevs, scf_string_t* debug_str, const char*   debug_info, size_t debug_info_size, scf_dwarf_info_header_t* header);
int scf_dwarf_info_encode(scf_dwarf_debug_t* debug, scf_dwarf_info_header_t* header);

void scf_dwarf_abbrev_print(scf_vector_t* abbrev_results);
void scf_dwarf_info_print  (scf_vector_t* infos);

const char* scf_dwarf_find_tag (const uint32_t type);
const char* scf_dwarf_find_form(const uint32_t type);
const char* scf_dwarf_find_attribute(const uint32_t type);

#endif

