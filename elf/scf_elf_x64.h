#ifndef SCF_ELF_X64_H
#define SCF_ELF_X64_H

#include"scf_elf.h"
#include"scf_vector.h"
#include"scf_string.h"

typedef struct scf_elf_x64_section_s  scf_elf_x64_section_t;

struct scf_elf_x64_section_s
{
	scf_elf_x64_section_t* link;
	scf_elf_x64_section_t* info;

	scf_string_t*	name;

	Elf64_Shdr		sh;

	uint64_t        offset;

	uint16_t		index;
	uint8_t*		data;
	int				data_len;
};

typedef struct {
	scf_elf_x64_section_t* section;

	scf_string_t*	name;

	Elf64_Sym		sym;

	int             index;
	uint8_t         dyn_flag:1;
} scf_elf_x64_sym_t;

typedef struct {
	Elf64_Ehdr      eh;

	Elf64_Shdr      sh_null;

	scf_vector_t*   sections;

	Elf64_Shdr		sh_symtab;
	scf_vector_t*	symbols;

	Elf64_Shdr		sh_strtab;

	Elf64_Shdr		sh_shstrtab;
	scf_string_t*   sh_shstrtab_data;

	scf_vector_t*	dynsyms;
	scf_vector_t*	dyn_needs;
	scf_vector_t*	dyn_relas;

	scf_elf_x64_section_t* interp;
	scf_elf_x64_section_t* dynsym;
	scf_elf_x64_section_t* dynstr;
	scf_elf_x64_section_t* gnu_version;
	scf_elf_x64_section_t* gnu_version_r;
	scf_elf_x64_section_t* rela_plt;
	scf_elf_x64_section_t* plt;
	scf_elf_x64_section_t* dynamic;
	scf_elf_x64_section_t* got_plt;

} scf_elf_x64_t;

#endif

