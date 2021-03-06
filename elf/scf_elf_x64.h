#ifndef SCF_ELF_X64_H
#define SCF_ELF_X64_H

#include"scf_elf.h"
#include"scf_vector.h"
#include"scf_string.h"

typedef struct {
	scf_string_t*	name;

	Elf64_Shdr		sh;

	uint16_t		index;
	uint8_t*		data;
	int				data_len;
} scf_elf_x64_section_t;

typedef struct {
	scf_string_t*	name;

	Elf64_Sym		sym;

	int             index;

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

} scf_elf_x64_t;

#endif

