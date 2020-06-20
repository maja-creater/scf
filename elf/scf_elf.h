#ifndef SCF_ELF_H
#define SCF_ELF_H

#include<elf.h>
#include"scf_list.h"

typedef struct scf_elf_context_s	scf_elf_context_t;
typedef struct scf_elf_ops_s		scf_elf_ops_t;

typedef struct {
	char*		name;

	uint8_t		st_info;
	uint16_t    st_shndx;

} scf_elf_sym_t;

typedef struct {
	char*		name;

	Elf64_Addr	r_offset;
	uint64_t	r_info;
	int64_t     r_addend;
} scf_elf_rela_t;

typedef struct {
	char*		name;

	uint8_t*	data;
	int			data_len;

	uint32_t	sh_type;
    uint64_t	sh_flags;
	uint64_t   sh_addralign;
} scf_elf_section_t;

struct scf_elf_ops_s {
	const char*		machine;

	int				(*open)(scf_elf_context_t* elf, const char* path);
	int				(*close)(scf_elf_context_t* elf);

	int				(*add_sym)(    scf_elf_context_t* elf, const scf_elf_sym_t*     sym);
	int				(*add_section)(scf_elf_context_t* elf, const scf_elf_section_t* section);
	int				(*add_rela)(   scf_elf_context_t* elf, const scf_elf_rela_t*    rela);

	int				(*write_rel)( scf_elf_context_t* elf, scf_list_t* code_list_head);
	int				(*write_dyn)( scf_elf_context_t* elf, scf_list_t* code_list_head);
	int				(*write_exec)(scf_elf_context_t* elf, scf_list_t* code_list_head);
};

struct scf_elf_context_s {

	scf_elf_ops_t*	ops;

	void*			priv;
};

int scf_elf_open( scf_elf_context_t** pelf, const char* machine, const char* path);
int scf_elf_close(scf_elf_context_t* elf);

int scf_elf_add_sym(    scf_elf_context_t* elf, const scf_elf_sym_t*     sym);
int scf_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section);
int scf_elf_add_rela(   scf_elf_context_t* elf, const scf_elf_rela_t*    rela);

int scf_elf_write_rel( scf_elf_context_t* elf, scf_list_t* code_list_head);
int scf_elf_write_dyn( scf_elf_context_t* elf, scf_list_t* code_list_head);
int scf_elf_write_exec(scf_elf_context_t* elf, scf_list_t* code_list_head);

#endif

