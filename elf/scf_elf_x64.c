#include"scf_elf_x64.h"

static int _x64_elf_open(scf_elf_context_t* elf, const char* path, const char* mode)
{
	if (!elf || !path || !mode)
		return -EINVAL;

	scf_elf_x64_t* x64 = calloc(1, sizeof(scf_elf_x64_t));
	if (!x64) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	x64->fp = fopen(path, mode);
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

	if (sym->name)
		s->name = scf_string_cstr(sym->name);
	else
		s->name = NULL;

	s->sym.st_size  = sym->st_size;
	s->sym.st_value = sym->st_value;
	s->sym.st_shndx = sym->st_shndx;
	s->sym.st_info	= sym->st_info;

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

	r->name			 = scf_string_cstr(rela->name);
	r->rela.r_offset = rela->r_offset;
	r->rela.r_info	 = rela->r_info;
	r->rela.r_addend = rela->r_addend;

	scf_vector_add(x64->relas, r);
	return 0;
}

static int _x64_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section)
{
	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_section_t* s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(section->name);
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->index 			= x64->sections->size + 1;
	s->sh.sh_type 		= section->sh_type;
	s->sh.sh_flags		= section->sh_flags;
	s->sh.sh_addralign	= section->sh_addralign;

	if (section->data && section->data_len > 0) {
		s->data = malloc(section->data_len);
		if (!s->data) {
			scf_string_free(s->name);
			free(s);
			return -ENOMEM;
		}

		memcpy(s->data, section->data, section->data_len);
		s->data_len = section->data_len;
	}

	if (scf_vector_add(x64->sections, s) < 0) {
		if (s->data)
			free(s->data);
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	return s->index;
}

static int _x64_elf_add_rela_section(scf_elf_context_t* elf, const scf_elf_section_t* section, scf_vector_t* relas)
{
	if (relas->size <= 0) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_section_t* s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(section->name);
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->index            = x64->sections->size + 1;
	s->sh.sh_type       = SHT_RELA;
	s->sh.sh_flags      = SHF_INFO_LINK;
	s->sh.sh_addralign  = section->sh_addralign;
	s->sh.sh_link       = section->sh_link;
	s->sh.sh_info       = section->sh_info;
	s->sh.sh_entsize    = sizeof(Elf64_Rela);

	s->data_len = sizeof(Elf64_Rela) * relas->size;

	s->data = malloc(s->data_len);
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}

	Elf64_Rela* pr = (Elf64_Rela*) s->data;

	int i;
	for (i = 0; i < relas->size; i++) {

		scf_elf_rela_t* r = relas->data[i];

		pr[i].r_offset = r->r_offset;
		pr[i].r_info   = r->r_info;
		pr[i].r_addend = r->r_addend;
	}

	if (scf_vector_add(x64->sections, s) < 0) {
		free(s->data);
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	return s->index;
}

static int _x64_elf_read_shstrtab(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64->fp)
		return -EINVAL;

	int ret = fseek(x64->fp, 0, SEEK_SET);
	if (ret < 0)
		return ret;

	ret = fread(&x64->eh, sizeof(Elf64_Ehdr), 1, x64->fp);
	if (ret != 1)
		return -1;

	if (ELFMAG0    != x64->eh.e_ident[EI_MAG0]
		|| ELFMAG1 != x64->eh.e_ident[EI_MAG1]
		|| ELFMAG2 != x64->eh.e_ident[EI_MAG2]
		|| ELFMAG3 != x64->eh.e_ident[EI_MAG3]) {

		scf_loge("not elf file\n");
		return -1;
	}

	long offset = x64->eh.e_shoff + x64->eh.e_shentsize * x64->eh.e_shstrndx;
	fseek(x64->fp, offset, SEEK_SET);

	ret = fread(&x64->sh_shstrtab, sizeof(Elf64_Shdr), 1, x64->fp);
	if (ret != 1)
		return -1;

	if (!x64->sh_shstrtab_data) {
		x64->sh_shstrtab_data = scf_string_alloc();
		if (!x64->sh_shstrtab_data)
			return -ENOMEM;
	}

	void* p = realloc(x64->sh_shstrtab_data->data, x64->sh_shstrtab.sh_size);
	if (!p)
		return -ENOMEM;
	x64->sh_shstrtab_data->data     = p;
	x64->sh_shstrtab_data->len      = x64->sh_shstrtab.sh_size;
	x64->sh_shstrtab_data->capacity = x64->sh_shstrtab.sh_size;

	scf_logd("x64->sh_shstrtab_data->data: %p\n",  x64->sh_shstrtab_data->data);
	scf_logd("x64->sh_shstrtab_data->len:  %ld\n", x64->sh_shstrtab_data->len);

	fseek(x64->fp, x64->sh_shstrtab.sh_offset, SEEK_SET);

	ret = fread(x64->sh_shstrtab_data->data, x64->sh_shstrtab.sh_size, 1, x64->fp);
	if (ret != 1)
		return -1;
#if 0
	int i;
	for (i = 0; i < x64->sh_shstrtab.sh_size; i++) {

		unsigned char c = x64->sh_shstrtab_data->data[i];
		if (c)
			printf("%c", c);
		else
			printf("\n");
	}
	printf("\n");
#endif
	return 0;
}

static int __x64_elf_read_section(scf_elf_context_t* elf, scf_elf_x64_section_t** psection, const char* name)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !x64->fp)
		return -1;

	if (!x64->sh_shstrtab_data) {

		int ret = _x64_elf_read_shstrtab(elf);
		if (ret < 0)
			return ret;
	}

	scf_elf_x64_section_t* s;
	int i;
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (!scf_string_cmp_cstr(s->name, name)) {

			if (s->data) {
				*psection = s;
				return 0;
			}

			goto _read_data;
		}
	}

	int j;
	for (j = 1; j < x64->eh.e_shnum; j++) {

		for (i = 0; i < x64->sections->size; i++) {
			s  =        x64->sections->data[i];

			if (j == s->index)
				break;
		}

		if (i < x64->sections->size)
			continue;

		s = calloc(1, sizeof(scf_elf_x64_section_t));
		if (!s)
			return -ENOMEM;

		long offset = x64->eh.e_shoff + x64->eh.e_shentsize * j;
		fseek(x64->fp, offset, SEEK_SET);

		int ret = fread(&s->sh, sizeof(Elf64_Shdr), 1, x64->fp);
		if (ret != 1) {
			free(s);
			return -1;
		}

		s->index = j;
		s->name  = scf_string_cstr(x64->sh_shstrtab_data->data + s->sh.sh_name);
		if (!s->name) {
			free(s);
			return -1;
		}

		ret = scf_vector_add(x64->sections, s);
		if (ret < 0) {
			scf_string_free(s->name);
			free(s);
			return -1;
		}

		if (!scf_string_cmp_cstr(s->name, name))
			break;
	}

	if (j < x64->eh.e_shnum) {

_read_data:
		if (!s->data) {

			s->data_len = s->sh.sh_size;

			if (s->sh.sh_size > 0) {

				s->data = malloc(s->sh.sh_size);
				if (!s->data)
					return -1;

				fseek(x64->fp, s->sh.sh_offset, SEEK_SET);

				int ret = fread(s->data, s->data_len, 1, x64->fp);
				if (ret != 1) {
					free(s->data);
					s->data = NULL;
					s->data_len = 0;
					return -1;
				}
			}
		} else
			assert(s->data_len == s->sh.sh_size);

		*psection = s;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _x64_elf_read_syms(scf_elf_context_t* elf, scf_vector_t* syms)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !x64->fp)
		return -1;

	scf_elf_x64_section_t* symtab = NULL;
	scf_elf_x64_section_t* strtab = NULL;

	int ret = __x64_elf_read_section(elf, &symtab, ".symtab");
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = __x64_elf_read_section(elf, &strtab, ".strtab");
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	assert(symtab->data_len % sizeof(Elf64_Sym) == 0);

	scf_elf_sym_t* esym;
	Elf64_Sym*     sym;
	int i;
	for (i  = 0; i < symtab->data_len; i += sizeof(Elf64_Sym)) {

		sym = (Elf64_Sym*)(symtab->data + i);

		assert(sym->st_name < strtab->data_len);

		if (STT_NOTYPE == sym->st_info)
			continue;

		esym = calloc(1, sizeof(scf_elf_sym_t));
		if (!esym)
			return -ENOMEM;

		esym->name     = strtab->data + sym->st_name;
		esym->st_size  = sym->st_size;
		esym->st_value = sym->st_value;
		esym->st_shndx = sym->st_shndx;
		esym->st_info  = sym->st_info;

		if (scf_vector_add(syms, esym) < 0) {
			free(esym);
			return -ENOMEM;
		}
	}

	return 0;
}

static int _x64_elf_read_relas(scf_elf_context_t* elf, scf_vector_t* relas)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !x64->fp)
		return -1;

	scf_elf_x64_section_t* rela_text = NULL;
	scf_elf_x64_section_t* symtab    = NULL;
	scf_elf_x64_section_t* strtab    = NULL;

	int ret = __x64_elf_read_section(elf, &symtab, ".symtab");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = __x64_elf_read_section(elf, &rela_text, ".rela.text");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = __x64_elf_read_section(elf, &strtab, ".strtab");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	assert(rela_text->data_len % sizeof(Elf64_Rela) == 0);

	scf_elf_rela_t* erela;
	Elf64_Rela*     rela;
	Elf64_Sym*      sym;

	int i;
	for (i   = 0; i < rela_text->data_len; i += sizeof(Elf64_Rela)) {

		rela = (Elf64_Rela*)(rela_text->data + i);

		int sym_idx = ELF64_R_SYM(rela->r_info);

		assert(sym_idx < symtab->data_len / sizeof(Elf64_Sym));

		sym = (Elf64_Sym*)(symtab->data + sym_idx * sizeof(Elf64_Sym));

		assert(sym->st_name < strtab->data_len);

		erela = calloc(1, sizeof(scf_elf_rela_t));
		if (!erela)
			return -ENOMEM;

		erela->name     = strtab->data + sym->st_name;
		erela->r_offset = rela->r_offset;
		erela->r_info   = rela->r_info;
		erela->r_addend = rela->r_addend;

		if (scf_vector_add(relas, erela) < 0) {
			scf_loge("\n");
			free(erela);
			return -ENOMEM;
		}
	}

	return 0;
}

static int _x64_elf_read_section(scf_elf_context_t* elf, scf_elf_section_t** psection, const char* name)
{
	scf_elf_x64_section_t* s   = NULL;
	scf_elf_section_t*     s2;
	scf_elf_x64_t*         x64 = elf->priv;

	if (!x64 || !x64->fp)
		return -1;

	int ret = __x64_elf_read_section(elf, &s, name);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	s2 = calloc(1, sizeof(scf_elf_section_t));
	if (!s2)
		return -ENOMEM;

	s2->index        = s->index;
	s2->name         = s->name->data;
	s2->data         = s->data;
	s2->data_len     = s->data_len;
	s2->sh_type      = s->sh.sh_type;
	s2->sh_flags     = s->sh.sh_flags;
	s2->sh_addralign = s->sh.sh_addralign;

	*psection = s2;
	return 0;
}

static void _x64_elf_header_fill(Elf64_Ehdr* eh, uint16_t e_type, Elf64_Addr e_entry,
		Elf64_Off e_phoff, uint16_t e_phnum, uint16_t e_shnum, uint16_t e_shstrndx)
{
	eh->e_ident[EI_MAG0]    = ELFMAG0;
	eh->e_ident[EI_MAG1]    = ELFMAG1;
	eh->e_ident[EI_MAG2]    = ELFMAG2;
	eh->e_ident[EI_MAG3]    = ELFMAG3;
	eh->e_ident[EI_CLASS]   = ELFCLASS64;
	eh->e_ident[EI_DATA]    = ELFDATA2LSB;
	eh->e_ident[EI_VERSION] = EV_CURRENT;
	eh->e_ident[EI_OSABI]   = ELFOSABI_SYSV;

	eh->e_type      = e_type;
	eh->e_machine   = EM_X86_64;
	eh->e_version   = EV_CURRENT;
	eh->e_entry     = e_entry;
	eh->e_ehsize    = sizeof(Elf64_Ehdr);

	eh->e_phoff     = e_phoff;
	eh->e_phentsize = sizeof(Elf64_Phdr);
	eh->e_phnum     = e_phnum;

	eh->e_shoff     = sizeof(Elf64_Ehdr);
	eh->e_shentsize = sizeof(Elf64_Shdr);
	eh->e_shnum     = e_shnum;
	eh->e_shstrndx  = e_shstrndx;
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

static int _x64_elf_write_rel(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64		= elf->priv;

	int 		nb_sections      = 1 + x64->sections->size + 1 + 1 + 1 + 1;
	uint64_t	shstrtab_offset  = 1;
	uint64_t	strtab_offset    = 1;
	Elf64_Off   section_offset   = sizeof(x64->eh) + sizeof(Elf64_Shdr) * nb_sections;

	// write elf header
	_x64_elf_header_fill(&x64->eh, ET_REL, 0, 0, 0, nb_sections, nb_sections - 1);
	fwrite(&x64->eh, sizeof(x64->eh), 1, x64->fp);

	// write null section header
	fwrite(&x64->sh_null, sizeof(x64->sh_null), 1, x64->fp);

	// write user's section header
	int i;
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		if (SHT_RELA == s->sh.sh_type)
			s->sh.sh_link = nb_sections - 3;

		_x64_elf_section_header_fill(&s->sh, shstrtab_offset, 0,
				section_offset, s->data_len,
				s->sh.sh_link,  s->sh.sh_info, s->sh.sh_entsize);
		s->sh.sh_addralign = 8;

		section_offset  += s->data_len;
		shstrtab_offset += s->name->len + 1;

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
	int  nb_local_syms = 1;

	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		if (sym->name) {
			sym->sym.st_name = strtab_offset;
			strtab_offset	 += sym->name->len + 1;
		} else
			sym->sym.st_name = 0;

		if (STB_LOCAL == ELF64_ST_BIND(sym->sym.st_info))
			nb_local_syms++;
	}

	scf_loge("x64->symbols->size: %d\n", x64->symbols->size);
	// write symtab section header
	_x64_elf_section_header_fill(&x64->sh_symtab, shstrtab_offset, 0,
			section_offset, (x64->symbols->size + 1) * sizeof(Elf64_Sym),
			nb_sections - 2, nb_local_syms, sizeof(Elf64_Sym));
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

		if (sym->name)
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

static int _sym_cmp(const void* v0, const void* v1)
{
	const scf_elf_x64_sym_t* sym0 = *(const scf_elf_x64_sym_t**)v0;
	const scf_elf_x64_sym_t* sym1 = *(const scf_elf_x64_sym_t**)v1;

	if (STB_LOCAL == ELF64_ST_BIND(sym0->sym.st_info)) {
		if (STB_GLOBAL == ELF64_ST_BIND(sym1->sym.st_info))
			return -1;
	} else if (STB_LOCAL == ELF64_ST_BIND(sym1->sym.st_info))
		return 1;
	return 0;
}

static int _x64_elf_write_exec(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64              = elf->priv;

	int 		   nb_sections      = 1 + x64->sections->size + 1 + 1 + 1;
	int 		   nb_phdrs         = 1;
	uint64_t	   shstrtab_offset  = 1;
	uint64_t	   strtab_offset    = 1;
	Elf64_Off      phdr_offset      = sizeof(x64->eh) + sizeof(Elf64_Shdr) * nb_sections;
	Elf64_Off      section_offset   = phdr_offset     + sizeof(Elf64_Phdr) * nb_phdrs;

	Elf64_Off      text_offset      = 0;
	uint64_t       text_len         = 0;

	Elf64_Off      data_offset      = 0;
	uint64_t       data_len         = 0;

	scf_elf_x64_section_t* s;
	scf_elf_x64_sym_t*     sym;

	int i;
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (!strcmp(".text", s->name->data)) {

			assert(0 == text_offset);
			assert(0 == text_len);

			text_offset = section_offset;
			text_len    = s->data_len;

			assert(text_len > 0);

		} else if (!strcmp(".data", s->name->data)) {

			assert(0 == data_offset);
			assert(0 == data_len);

			data_offset = section_offset;
			data_len    = s->data_len;

			assert(data_len > 0);
		}

		section_offset  += s->data_len;
	}

	uint64_t _start =  0;

	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		if (!strcmp(sym->name->data, "_start")) {

			if (0 != _start) {
				scf_loge("\n");
				return -EINVAL;
			}

			_start = sym->sym.st_value;
			break;
		}
	}

	// write elf header
	_x64_elf_header_fill(&x64->eh, ET_EXEC, _start + text_offset + 0x400000, phdr_offset, nb_phdrs, nb_sections, nb_sections - 1);
	fwrite(&x64->eh, sizeof(x64->eh), 1, x64->fp);

	// write null section header
	fwrite(&x64->sh_null, sizeof(x64->sh_null), 1, x64->fp);

	// write user's section header
	section_offset   = phdr_offset + sizeof(Elf64_Phdr) * nb_phdrs;

	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (SHT_RELA == s->sh.sh_type)
			s->sh.sh_link = nb_sections - 3;

		_x64_elf_section_header_fill(&s->sh, shstrtab_offset, 0,
				section_offset, s->data_len,
				s->sh.sh_link,  s->sh.sh_info, s->sh.sh_entsize);
		s->sh.sh_addralign = 8;

		section_offset  += s->data_len;
		shstrtab_offset += s->name->len + 1;

		fwrite(&s->sh, sizeof(s->sh), 1, x64->fp);
	}

	// set user's symbols' name
	int  nb_local_syms = 1;

	qsort(x64->symbols->data, x64->symbols->size, sizeof(void*), _sym_cmp);

	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		if (sym->name) {
			sym->sym.st_name = strtab_offset;
			strtab_offset	 += sym->name->len + 1;
		} else
			sym->sym.st_name = 0;

		if (STB_LOCAL == ELF64_ST_BIND(sym->sym.st_info))
			nb_local_syms++;
	}

	scf_loge("x64->symbols->size: %d\n", x64->symbols->size);
	// write symtab section header
	_x64_elf_section_header_fill(&x64->sh_symtab, shstrtab_offset, 0,
			section_offset, (x64->symbols->size + 1) * sizeof(Elf64_Sym),
			nb_sections - 2, nb_local_syms, sizeof(Elf64_Sym));
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

	// write program header
	Elf64_Phdr ph_text = {0};
	Elf64_Phdr ph_data = {0};
	Elf64_Phdr ph_stack= {0};

	scf_loge("text_offset: %#lx, text_len: %#lx\n", text_offset, text_len);

	ph_text.p_type     = PT_LOAD;
	ph_text.p_flags    = PF_R | PF_X;
	ph_text.p_offset   = 0;
	ph_text.p_vaddr    = 0x400000;
	ph_text.p_paddr    = ph_text.p_vaddr;
	ph_text.p_filesz   = text_offset + text_len;
	ph_text.p_memsz    = ph_text.p_filesz;
	ph_text.p_align    = 0x200000;

	fwrite(&ph_text,  sizeof(ph_text),  1, x64->fp);
#if 0
	ph_data.p_type     = PT_LOAD;
	ph_data.p_flags    = PF_R | PF_W;
	ph_data.p_offset   = 0;
	ph_data.p_vaddr    = ph_text.p_vaddr + ph_text.p_align + data_offset;
	ph_data.p_paddr    = ph_data.p_vaddr;
	ph_data.p_filesz   = data_len;
	ph_data.p_memsz    = ph_data.p_filesz;
	ph_data.p_align    = 0x200000;

	ph_stack.p_type    = PT_GNU_STACK;
	ph_stack.p_flags   = PF_R | PF_W;
	ph_stack.p_offset  = 0;
	ph_stack.p_vaddr   = 0;
	ph_stack.p_paddr   = 0;
	ph_stack.p_filesz  = 0;
	ph_stack.p_memsz   = 0;
	ph_stack.p_align   = 0x10;

	fwrite(&ph_data,  sizeof(ph_data),  1, x64->fp);
	fwrite(&ph_stack, sizeof(ph_stack), 1, x64->fp);
#endif

	// write user's section data
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

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

	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

//		sym->sym.st_value += 0x400000 + text_offset;

		fwrite(&sym->sym, sizeof(sym->sym), 1, x64->fp);
	}

	// write strtab data (strtab section, symbol names of symtab)
	uint8_t c = 0;
	fwrite(&c, sizeof(c), 1, x64->fp);
	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		if (sym->name)
			fwrite(sym->name->data, sym->name->len + 1, 1, x64->fp);
	}

	// write shstrtab data (shstrtab section, section names of all sections)
	fwrite(&c, sizeof(c), 1, x64->fp);
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		fwrite(s->name->data, s->name->len + 1, 1, x64->fp);
	}

	char* str   = ".symtab\0.strtab\0.shstrtab\0";
	int str_len = strlen(".symtab") + strlen(".strtab") + strlen(".shstrtab") + 4;
	fwrite(str, str_len, 1, x64->fp);
	return 0;
}

scf_elf_ops_t	elf_ops_x64 =
{
	.machine	      = "x64",

	.open		      = _x64_elf_open,
	.close		      = _x64_elf_close,

	.add_sym	      = _x64_elf_add_sym,
	.add_section      = _x64_elf_add_section,
	.add_rela	      = _x64_elf_add_rela,

	.add_rela_section = _x64_elf_add_rela_section,

	.read_syms        = _x64_elf_read_syms,
	.read_relas       = _x64_elf_read_relas,
	.read_section     = _x64_elf_read_section,

	.write_rel	      = _x64_elf_write_rel,
	.write_exec       = _x64_elf_write_exec,
};

