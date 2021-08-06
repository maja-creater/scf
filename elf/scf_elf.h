#ifndef SCF_ELF_H
#define SCF_ELF_H

#include<elf.h>
#include"scf_list.h"
#include"scf_vector.h"

typedef struct scf_elf_context_s	scf_elf_context_t;
typedef struct scf_elf_ops_s		scf_elf_ops_t;

typedef struct {
	char*		name;
	uint64_t    st_size;
	Elf64_Addr  st_value;

	uint16_t    st_shndx;
	uint8_t		st_info;

	uint8_t     dyn_flag:1;
} scf_elf_sym_t;

typedef struct {
	char*		name;

	Elf64_Addr	r_offset;
	uint64_t	r_info;
	int64_t     r_addend;
} scf_elf_rela_t;

typedef struct {
	char*		name;

	uint32_t    index;

	uint8_t*	data;
	int			data_len;

	uint32_t	sh_type;
    uint64_t	sh_flags;
	uint64_t    sh_addralign;
	uint32_t    sh_link;
	uint32_t    sh_info;
} scf_elf_section_t;

struct scf_elf_ops_s
{
	const char*		machine;

	int				(*open )(scf_elf_context_t* elf);
	int				(*close)(scf_elf_context_t* elf);

	int				(*add_sym   )(scf_elf_context_t* elf, const scf_elf_sym_t*  sym,   const char* sh_name);
	int				(*read_syms )(scf_elf_context_t* elf,       scf_vector_t*   syms,  const char* sh_name);
	int				(*read_relas)(scf_elf_context_t* elf,       scf_vector_t*   relas, const char* sh_name);

	int				(*add_section )(scf_elf_context_t* elf, const scf_elf_section_t*  section);
	int				(*read_section)(scf_elf_context_t* elf,       scf_elf_section_t** psection, const char* name);

	int				(*add_rela_section)(scf_elf_context_t* elf, const scf_elf_section_t* section, scf_vector_t* relas);

	int				(*add_dyn_need)(scf_elf_context_t* elf, const char* soname);
	int	            (*add_dyn_rela)(scf_elf_context_t* elf, const scf_elf_rela_t* rela);

	int				(*write_rel )(scf_elf_context_t* elf);
	int				(*write_exec)(scf_elf_context_t* elf);
};

struct scf_elf_context_s {

	scf_elf_ops_t*	ops;

	void*			priv;

	FILE*           fp;
	int64_t         start;
	int64_t         end;
};

void scf_elf_rela_free(scf_elf_rela_t* rela);

int scf_elf_open (scf_elf_context_t** pelf, const char* machine, const char* path, const char* mode);
int scf_elf_open2(scf_elf_context_t*  elf,  const char* machine);
int scf_elf_close(scf_elf_context_t*  elf);

int scf_elf_add_sym (scf_elf_context_t* elf, const scf_elf_sym_t*     sym, const char* sh_name);

int scf_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section);

int	scf_elf_add_rela_section(scf_elf_context_t* elf, const scf_elf_section_t* section, scf_vector_t* relas);
int	scf_elf_add_dyn_need(scf_elf_context_t* elf, const char* soname);
int	scf_elf_add_dyn_rela(scf_elf_context_t* elf, const scf_elf_rela_t* rela);

int scf_elf_read_section(scf_elf_context_t* elf, scf_elf_section_t** psection, const char* name);

int scf_elf_read_syms (scf_elf_context_t* elf, scf_vector_t* syms,  const char* sh_name);
int scf_elf_read_relas(scf_elf_context_t* elf, scf_vector_t* relas, const char* sh_name);

int scf_elf_write_rel( scf_elf_context_t* elf);
int scf_elf_write_exec(scf_elf_context_t* elf);

#endif

