#include"scf_elf_x64.h"

static int _x64_elf_open(scf_elf_context_t* elf, const char* path)
{
	scf_elf_x64_t* x64 = calloc(1, sizeof(scf_elf_x64_t));
	if (!x64) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	x64->fp = fopen(path, "wb");
	if (!x64->fp) {
		printf("%s(),%d, error\n", __func__, __LINE__);

		free(x64);
		x64 = NULL;
		return -1;
	}

	x64->sh_null.sh_type			= SHT_NULL;

	x64->sh_rela.sh_type			= SHT_RELA;
	x64->sh_rela.sh_flags			= SHF_INFO_LINK;
	x64->sh_rela.sh_addralign 		= 8;

	x64->sh_symtab.sh_type			= SHT_SYMTAB;
	x64->sh_symtab.sh_flags			= 0;
	x64->sh_symtab.sh_addralign 	= 8;

	x64->sh_strtab.sh_type			= SHT_STRTAB;
	x64->sh_strtab.sh_flags			= 0;
	x64->sh_strtab.sh_addralign 	= 1;

	x64->sh_shstrtab.sh_type		= SHT_STRTAB;
	x64->sh_shstrtab.sh_flags		= 0;
	x64->sh_shstrtab.sh_addralign 	= 1;

	x64->sections = scf_vector_alloc();
	x64->symbols  = scf_vector_alloc();
	x64->relas	  = scf_vector_alloc();

	elf->priv = x64;
	return 0;
}

static int _x64_elf_close(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (x64) {
		fclose(x64->fp);
		free(x64);
		x64 = NULL;
	}
	return 0;
}

static int _x64_elf_add_sym(scf_elf_context_t* elf, const scf_elf_sym_t* sym)
{
	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_sym_t* s = calloc(1, sizeof(scf_elf_x64_sym_t));
	if (!s) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	s->name			= scf_string_cstr(sym->name);
	s->sym.st_info	= sym->st_info;
	s->sym.st_shndx = sym->st_shndx;

	scf_vector_add(x64->symbols, s);
	return 0;
}

static int _x64_elf_add_rela(scf_elf_context_t* elf, const scf_elf_rela_t* rela)
{
	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_rela_t* r = calloc(1, sizeof(scf_elf_x64_rela_t));
	if (!r) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	r->name			= scf_string_cstr(rela->name);
	r->rela.r_offset= rela->r_offset;
	r->rela.r_info	= rela->r_info;
	r->rela.r_addend= rela->r_addend;

	scf_vector_add(x64->relas, r);
	return 0;
}

static int _x64_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section)
{
	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_section_t* s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	s->name = scf_string_cstr(section->name);
	if (!s->name) {
		printf("%s(),%d, error\n", __func__, __LINE__);

		free(s);
		s = NULL;
		return -1;
	}

	s->index 			= x64->sections->size + 1;
	s->sh.sh_type 		= section->sh_type;
	s->sh.sh_flags		= section->sh_flags;
	s->sh.sh_addralign	= section->sh_addralign;

	if (section->data && section->data_len > 0) {
		s->data = malloc(section->data_len);
		if (!s->data) {
			printf("%s(),%d, error\n", __func__, __LINE__);
			return -1;
		}

		memcpy(s->data, section->data, section->data_len);
		s->data_len = section->data_len;
	}

	scf_vector_add(x64->sections, s);
	return 0;
}

static void _x64_elf_header_fill(Elf64_Ehdr* eh, Elf64_Addr e_entry, uint16_t e_shnum, uint16_t e_shstrndx)
{
	eh->e_ident[EI_MAG0] = ELFMAG0;
	eh->e_ident[EI_MAG1] = ELFMAG1;
	eh->e_ident[EI_MAG2] = ELFMAG2;
	eh->e_ident[EI_MAG3] = ELFMAG3;
	eh->e_ident[EI_CLASS] = ELFCLASS64;
	eh->e_ident[EI_DATA] = ELFDATA2LSB;
	eh->e_ident[EI_VERSION] = EV_CURRENT;
	eh->e_ident[EI_OSABI] = ELFOSABI_SYSV;

	eh->e_type = ET_REL;
	eh->e_machine = EM_X86_64;
	eh->e_version = EV_CURRENT;
	eh->e_entry = e_entry;
	eh->e_phoff = 0;
	eh->e_ehsize = sizeof(Elf64_Ehdr);

	eh->e_shoff = sizeof(Elf64_Ehdr);
	eh->e_shentsize = sizeof(Elf64_Shdr);
	eh->e_shnum = e_shnum;
	eh->e_shstrndx = e_shstrndx;
}

static void _x64_elf_section_header_fill(Elf64_Shdr* sh,
		uint32_t   sh_name,
        Elf64_Addr sh_addr,
        Elf64_Off  sh_offset,
        uint64_t   sh_size,
        uint32_t   sh_link,
        uint32_t   sh_info,
        uint64_t   sh_entsize)
{
	sh->sh_name		= sh_name;
	sh->sh_addr		= sh_addr;
	sh->sh_offset	= sh_offset;
	sh->sh_size		= sh_size;
	sh->sh_link		= sh_link;
	sh->sh_info		= sh_info;
	sh->sh_entsize	= sh_entsize;
}

static int _x64_elf_write_rel(scf_elf_context_t* elf, scf_list_t* code_list_head)
{
	scf_elf_x64_t* x64		= elf->priv;

	int 		nb_sections      = 1 + x64->sections->size + 1 + 1 + 1 + 1;
	uint64_t	shstrtab_offset  = 1;
	uint64_t	strtab_offset    = 1;
	Elf64_Off   section_offset   = sizeof(x64->eh) + sizeof(Elf64_Shdr) * nb_sections;

	// write elf header
	_x64_elf_header_fill(&x64->eh, 0, nb_sections, nb_sections - 1);
	fwrite(&x64->eh, sizeof(x64->eh), 1, x64->fp);

	// write null section header
	fwrite(&x64->sh_null, sizeof(x64->sh_null), 1, x64->fp);

	// write user's section header
	int i;
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		_x64_elf_section_header_fill(&s->sh, shstrtab_offset, 0,
				section_offset, s->data_len,
				0, 0, 0);
		s->sh.sh_addralign = 8;

		section_offset += s->data_len;
		shstrtab_offset  += s->name->len + 1;

		fwrite(&s->sh, sizeof(s->sh), 1, x64->fp);
	}

	// write rela section header
	_x64_elf_section_header_fill(&x64->sh_rela, shstrtab_offset, 0,
			section_offset, x64->relas->size * sizeof(Elf64_Rela),
			nb_sections - 3, 1, sizeof(Elf64_Rela));
	fwrite(&x64->sh_rela, sizeof(x64->sh_rela), 1, x64->fp);
	section_offset   += x64->relas->size * sizeof(Elf64_Rela);
	shstrtab_offset  += strlen(".rela.text") + 1;

	// set user's symbols' name
	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		sym->sym.st_name = strtab_offset;
		strtab_offset	 += sym->name->len + 1;
	}

	// write symtab section header
	_x64_elf_section_header_fill(&x64->sh_symtab, shstrtab_offset, 0,
			section_offset, (x64->symbols->size + 1) * sizeof(Elf64_Sym),
			nb_sections - 2, 1, sizeof(Elf64_Sym));
	fwrite(&x64->sh_symtab, sizeof(x64->sh_symtab), 1, x64->fp);
	section_offset   += (x64->symbols->size + 1) * sizeof(Elf64_Sym);
	shstrtab_offset  += strlen(".symtab") + 1;

	// write strtab section header
	_x64_elf_section_header_fill(&x64->sh_strtab, shstrtab_offset, 0,
			section_offset, strtab_offset,
			0, 0, 0);
	fwrite(&x64->sh_strtab, sizeof(x64->sh_strtab), 1, x64->fp);
	section_offset   += strtab_offset;
	shstrtab_offset  += strlen(".strtab") + 1;

	// write shstrtab section header
	uint64_t shstrtab_len = shstrtab_offset + strlen(".shstrtab") + 1;
	_x64_elf_section_header_fill(&x64->sh_shstrtab, shstrtab_offset, 0,
			section_offset, shstrtab_len, 0, 0, 0);
	fwrite(&x64->sh_shstrtab, sizeof(x64->sh_shstrtab), 1, x64->fp);

	// write user's section data
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		if (s->data && s->data_len > 0)
			fwrite(s->data, s->data_len, 1, x64->fp);
	}

	// write user's relas data (rela section)
	for (i = 0; i < x64->relas->size; i++) {
		scf_elf_x64_rela_t* r = x64->relas->data[i];
		fwrite(&r->rela, sizeof(r->rela), 1, x64->fp);
	}

	// write user's symbols data (symtab section)
	// entry index 0 in symtab is NOTYPE
	Elf64_Sym sym0 = {0};
	sym0.st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
	fwrite(&sym0, sizeof(sym0), 1, x64->fp);

	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		fwrite(&sym->sym, sizeof(sym->sym), 1, x64->fp);
	}

	// write strtab data (strtab section, symbol names of symtab)
	uint8_t c = 0;
	fwrite(&c, sizeof(c), 1, x64->fp);
	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		fwrite(sym->name->data, sym->name->len + 1, 1, x64->fp);
	}

	// write shstrtab data (shstrtab section, section names of all sections)
	fwrite(&c, sizeof(c), 1, x64->fp);
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		fwrite(s->name->data, s->name->len + 1, 1, x64->fp);
	}

	char* str = ".rela.text\0.symtab\0.strtab\0.shstrtab\0";
	int str_len = strlen(".rela.text") + strlen(".symtab") + strlen(".strtab") + strlen(".shstrtab") + 4;
	fwrite(str, str_len, 1, x64->fp);
	return 0;
}

scf_elf_ops_t	elf_ops_x64 = {

	.machine	 = "x64",

	.open		 = _x64_elf_open,
	.close		 = _x64_elf_close,

	.add_sym	 = _x64_elf_add_sym,
	.add_section = _x64_elf_add_section,
	.add_rela	 = _x64_elf_add_rela,

	.write_rel	 = _x64_elf_write_rel,
};

