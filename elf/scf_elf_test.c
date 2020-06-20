#include"scf_elf.h"

int main()
{
	scf_elf_context_t* elf = NULL;

	if (scf_elf_open(&elf, "x64", "./1.elf") < 0) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_elf_section_t*	section = calloc(1, sizeof(scf_elf_section_t));
	assert(section);
	scf_elf_sym_t*		sym		= calloc(1, sizeof(scf_elf_sym_t));
	assert(sym);
	scf_elf_rela_t*		rela		= calloc(1, sizeof(scf_elf_rela_t));
	assert(rela);

	uint8_t call[] = {
		0x55,
		0x48, 0x89, 0xe5,
		0x48, 0x8d, 0x3d, 0, 0, 0, 0,
		0xe8, 0, 0, 0, 0,
		0x5d,
		0xc3
	};

	section->name		 	= ".text";
	section->sh_type		= SHT_PROGBITS;
	section->sh_flags	 	= SHF_ALLOC | SHF_EXECINSTR;
	section->sh_addralign	= 1;
	section->data			= call;
	section->data_len		= sizeof(call) / sizeof(call[0]);
	scf_elf_add_section(elf, section);

	uint8_t* msg = "hello\n";
	section->name		 	= ".rodata";
	section->sh_type		= SHT_PROGBITS;
	section->sh_flags	 	= SHF_ALLOC;
	section->sh_addralign	= 8;
	section->data			= msg;
	section->data_len		= strlen(msg) + 1;
	scf_elf_add_section(elf, section);

	sym->name		= "main";
	sym->st_info	= ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
	sym->st_shndx   = 1; 
	scf_elf_add_sym(elf, sym);

	sym->name		= "printf";
	sym->st_info	= ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
	sym->st_shndx   = 0;
	scf_elf_add_sym(elf, sym);
	rela->name		= "printf";
	rela->r_offset	= 12;
	rela->r_info	= ELF64_R_INFO(2, R_X86_64_PC32);
	rela->r_addend	= -4;
	scf_elf_add_rela(elf, rela);

	sym->name		= ".rodata";
	sym->st_info	= ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
	sym->st_shndx   = 2;
	scf_elf_add_sym(elf, sym);
	rela->name		= ".rodata";
	rela->r_offset	= 7;
	rela->r_info	= ELF64_R_INFO(3, R_X86_64_PC32);
	rela->r_addend	= -4;
	scf_elf_add_rela(elf, rela);

	scf_elf_write_rel(elf, NULL);

	scf_elf_close(elf);
	elf = NULL;
	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

