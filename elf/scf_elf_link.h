#ifndef SCF_ELF_LINK_H
#define SCF_ELF_LINK_H

#include"scf_elf.h"
#include"scf_string.h"
#include<ar.h>

typedef struct {
	scf_elf_context_t* elf;

	scf_string_t*      name;

	int                text_idx;
	int                rodata_idx;
	int                data_idx;

	int                abbrev_idx;
	int                info_idx;
	int                line_idx;
	int                str_idx;

	scf_string_t*      text;
	scf_string_t*      rodata;
	scf_string_t*      data;

	scf_string_t*      debug_abbrev;
	scf_string_t*      debug_info;
	scf_string_t*      debug_line;
	scf_string_t*      debug_str;

	scf_vector_t*      syms;

	scf_vector_t*      text_relas;
	scf_vector_t*      data_relas;

	scf_vector_t*      debug_line_relas;
	scf_vector_t*      debug_info_relas;

	scf_vector_t*      dyn_syms;
	scf_vector_t*      rela_plt;
	scf_vector_t*      dyn_needs;

} scf_elf_file_t;

#define SCF_ELF_FILE_SHNDX(member) ((void**)&((scf_elf_file_t*)0)->member - (void**)&((scf_elf_file_t*)0)->text + 1)

typedef struct {
	scf_string_t*      name;
	uint32_t           offset;

} scf_ar_sym_t;

typedef struct {

	scf_vector_t*      symbols;
	scf_vector_t*      files;

	FILE*              fp;

} scf_ar_file_t;

int scf_elf_file_close(scf_elf_file_t* ef, void (*rela_free)(void*), void (*sym_free)(void*));

int scf_elf_link(scf_vector_t* objs, scf_vector_t* afiles, scf_vector_t* sofiles, const char* out);

#endif

