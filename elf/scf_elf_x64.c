#include"scf_elf_x64.h"
#include"scf_elf_link.h"

static int _x64_elf_open(scf_elf_context_t* elf)
{
	if (!elf)
		return -EINVAL;

	scf_elf_x64_t* x64 = calloc(1, sizeof(scf_elf_x64_t));
	if (!x64)
		return -ENOMEM;

	x64->sh_null.sh_type          = SHT_NULL;

	x64->sh_symtab.sh_type        = SHT_SYMTAB;
	x64->sh_symtab.sh_flags       = 0;
	x64->sh_symtab.sh_addralign   = 8;

	x64->sh_strtab.sh_type        = SHT_STRTAB;
	x64->sh_strtab.sh_flags       = 0;
	x64->sh_strtab.sh_addralign   = 1;

	x64->sh_shstrtab.sh_type      = SHT_STRTAB;
	x64->sh_shstrtab.sh_flags     = 0;
	x64->sh_shstrtab.sh_addralign = 1;

	x64->sections = scf_vector_alloc();
	x64->symbols  = scf_vector_alloc();

	elf->priv = x64;
	return 0;
}

static int _x64_elf_close(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (x64) {
		free(x64);
		x64 = NULL;
	}
	return 0;
}

static int _x64_elf_add_sym(scf_elf_context_t* elf, const scf_elf_sym_t* sym, const char* sh_name)
{
	scf_elf_x64_sym_t* xsym;
	scf_elf_x64_t*     x64 = elf->priv;
	scf_vector_t*      vec = NULL;

	if (!strcmp(sh_name, ".symtab"))
		vec = x64->symbols;

	else if (!strcmp(sh_name, ".dynsym")) {

		if (!x64->dynsyms) {
			x64->dynsyms = scf_vector_alloc();
			if (!x64->dynsyms)
				return -ENOMEM;
		}

		vec = x64->dynsyms;
	} else
		return -EINVAL;

	xsym = calloc(1, sizeof(scf_elf_x64_sym_t));
	if (!xsym)
		return -ENOMEM;

	if (sym->name)
		xsym->name = scf_string_cstr(sym->name);
	else
		xsym->name = NULL;

	xsym->sym.st_size  = sym->st_size;
	xsym->sym.st_value = sym->st_value;
	xsym->sym.st_shndx = sym->st_shndx;
	xsym->sym.st_info  = sym->st_info;

	xsym->dyn_flag     = sym->dyn_flag;

	int ret = scf_vector_add(vec, xsym);
	if (ret < 0) {
		scf_string_free(xsym->name);
		free(xsym);
		return ret;
	}

	xsym->index = vec->size;
	return 0;
}

static int _x64_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section)
{
	scf_elf_x64_t* x64 = elf->priv;

	scf_elf_x64_section_t* s;
	scf_elf_x64_section_t* s2;
	int i;

	if (section->index > 0) {

		for (i = x64->sections->size - 1; i >= 0; i--) {
			s  = x64->sections->data[i];

			if (s->index == section->index) {
				scf_loge("s->index: %d\n", s->index);
				return -1;
			}
		}
	}

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(section->name);
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

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

	if (0 == section->index)
		s->index = x64->sections->size;
	else {
		s->index = section->index;

		for (i = x64->sections->size - 2; i >= 0; i--) {
			s2  = x64->sections->data[i];

			if (s2->index < s->index)
				break;

			x64->sections->data[i + 1] = s2;
		}

		x64->sections->data[i + 1] = s;
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

	if (!elf->fp)
		return -EINVAL;

	int ret = fseek(elf->fp, elf->start, SEEK_SET);
	if (ret < 0)
		return ret;

	ret = fread(&x64->eh, sizeof(Elf64_Ehdr), 1, elf->fp);
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
	fseek(elf->fp, elf->start + offset, SEEK_SET);

	ret = fread(&x64->sh_shstrtab, sizeof(Elf64_Shdr), 1, elf->fp);
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

	fseek(elf->fp, elf->start + x64->sh_shstrtab.sh_offset, SEEK_SET);

	ret = fread(x64->sh_shstrtab_data->data, x64->sh_shstrtab.sh_size, 1, elf->fp);
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

static int __x64_elf_read_section_data(scf_elf_context_t* elf, scf_elf_x64_section_t* s)
{
	s->data_len = s->sh.sh_size;

	if (s->sh.sh_size > 0) {

		s->data = malloc(s->sh.sh_size);
		if (!s->data)
			return -1;

		fseek(elf->fp, elf->start + s->sh.sh_offset, SEEK_SET);

		int ret = fread(s->data, s->data_len, 1, elf->fp);
		if (ret != 1) {
			free(s->data);
			s->data = NULL;
			s->data_len = 0;
			return -1;
		}
	}

	return 0;
}

static int __x64_elf_read_section_by_index(scf_elf_context_t* elf, scf_elf_x64_section_t** psection, const int index)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
		return -1;

	if (!x64->sh_shstrtab_data) {

		int ret = _x64_elf_read_shstrtab(elf);
		if (ret < 0)
			return ret;
	}

	if (index >= x64->eh.e_shnum)
		return -EINVAL;

	scf_elf_x64_section_t* s;
	int i;
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (index == s->index) {

			if (s->data || __x64_elf_read_section_data(elf, s) == 0) {
				*psection = s;
				return 0;
			}
			return -1;
		}
	}

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	long offset = x64->eh.e_shoff + x64->eh.e_shentsize * index;
	fseek(elf->fp, elf->start + offset, SEEK_SET);

	int ret = fread(&s->sh, sizeof(Elf64_Shdr), 1, elf->fp);
	if (ret != 1) {
		free(s);
		return -1;
	}

	s->index = index;
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

	if (__x64_elf_read_section_data(elf, s) == 0) {
		*psection = s;
		return 0;
	}
	return -1;
}

static int __x64_elf_read_section(scf_elf_context_t* elf, scf_elf_x64_section_t** psection, const char* name)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
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

			if (s->data || __x64_elf_read_section_data(elf, s) == 0) {
				*psection = s;
				return 0;
			}
			return -1;
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
		fseek(elf->fp, elf->start + offset, SEEK_SET);

		int ret = fread(&s->sh, sizeof(Elf64_Shdr), 1, elf->fp);
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

		if (!s->data) {
			if (__x64_elf_read_section_data(elf, s) == 0) {
				*psection = s;
				return 0;
			}

			return -1;
		} else
			assert(s->data_len == s->sh.sh_size);

		*psection = s;
		return 0;
	}

	return -404;
}

static int _x64_elf_read_syms(scf_elf_context_t* elf, scf_vector_t* syms, const char* sh_name)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
		return -1;

	scf_elf_x64_section_t* symtab = NULL;
	scf_elf_x64_section_t* strtab = NULL;

	char* sh_strtab_name = NULL;

	if (!strcmp(sh_name, ".symtab"))
		sh_strtab_name = ".strtab";

	else if (!strcmp(sh_name, ".dynsym"))
		sh_strtab_name = ".dynstr";
	else
		return -EINVAL;

	int ret = __x64_elf_read_section(elf, &symtab, sh_name);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = __x64_elf_read_section(elf, &strtab, sh_strtab_name);
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

static int _x64_elf_add_dyn_need(scf_elf_context_t* elf, const char* soname)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
		return -1;

	if (!x64->dyn_needs) {
		x64->dyn_needs = scf_vector_alloc();
		if (!x64->dyn_needs)
			return -ENOMEM;
	}

	scf_string_t* s = scf_string_cstr(soname);
	if (!s)
		return -ENOMEM;

	if (scf_vector_add(x64->dyn_needs, s) < 0) {
		scf_string_free(s);
		return -ENOMEM;
	}

	scf_loge("soname: %s\n", soname);
	return 0;
}

static int _x64_elf_add_dyn_rela(scf_elf_context_t* elf, const scf_elf_rela_t* rela)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
		return -1;

	if (!x64->dyn_relas) {
		x64->dyn_relas = scf_vector_alloc();
		if (!x64->dyn_relas)
			return -ENOMEM;
	}

	Elf64_Rela* r = calloc(1, sizeof(Elf64_Rela));
	if (!r)
		return -ENOMEM;

	if (scf_vector_add(x64->dyn_relas, r) < 0) {
		free(r);
		return -ENOMEM;
	}

	r->r_offset = rela->r_offset;
	r->r_addend = rela->r_addend;
	r->r_info   = rela->r_info;

	return 0;
}

static int _x64_elf_read_relas(scf_elf_context_t* elf, scf_vector_t* relas, const char* sh_name)
{
	scf_elf_x64_t* x64 = elf->priv;

	if (!x64 || !elf->fp)
		return -1;

	scf_elf_x64_section_t* sh_rela  = NULL;
	scf_elf_x64_section_t* symtab   = NULL;
	scf_elf_x64_section_t* strtab   = NULL;

	char* symtab_name = NULL;
	char* strtab_name = NULL;

	int ret = __x64_elf_read_section(elf, &sh_rela, sh_name);
	if (ret < 0)
		return ret;

	scf_loge("sh_rela: %u\n", sh_rela->sh.sh_link);

	ret = __x64_elf_read_section_by_index(elf, &symtab, sh_rela->sh.sh_link);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = __x64_elf_read_section_by_index(elf, &strtab, symtab->sh.sh_link);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	assert(sh_rela->data_len % sizeof(Elf64_Rela) == 0);

	scf_elf_rela_t* erela;
	Elf64_Rela*     rela;
	Elf64_Sym*      sym;

	int i;
	for (i   = 0; i < sh_rela->data_len; i += sizeof(Elf64_Rela)) {

		rela = (Elf64_Rela*)(sh_rela->data + i);

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

	if (!x64 || !elf->fp)
		return -1;

	int ret = __x64_elf_read_section(elf, &s, name);
	if (ret < 0)
		return ret;

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

	int 		nb_sections      = 1 + x64->sections->size + 1 + 1 + 1;
	uint64_t	shstrtab_offset  = 1;
	uint64_t	strtab_offset    = 1;
	Elf64_Off   section_offset   = sizeof(x64->eh) + sizeof(Elf64_Shdr) * nb_sections;

	// write elf header
	_x64_elf_header_fill(&x64->eh, ET_REL, 0, 0, 0, nb_sections, nb_sections - 1);
	fwrite(&x64->eh, sizeof(x64->eh), 1, elf->fp);

	// write null section header
	fwrite(&x64->sh_null, sizeof(x64->sh_null), 1, elf->fp);

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

		fwrite(&s->sh, sizeof(s->sh), 1, elf->fp);
	}

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
	fwrite(&x64->sh_symtab, sizeof(x64->sh_symtab), 1, elf->fp);
	section_offset   += (x64->symbols->size + 1) * sizeof(Elf64_Sym);
	shstrtab_offset  += strlen(".symtab") + 1;

	// write strtab section header
	_x64_elf_section_header_fill(&x64->sh_strtab, shstrtab_offset, 0,
			section_offset, strtab_offset,
			0, 0, 0);
	fwrite(&x64->sh_strtab, sizeof(x64->sh_strtab), 1, elf->fp);
	section_offset   += strtab_offset;
	shstrtab_offset  += strlen(".strtab") + 1;

	// write shstrtab section header
	uint64_t shstrtab_len = shstrtab_offset + strlen(".shstrtab") + 1;
	_x64_elf_section_header_fill(&x64->sh_shstrtab, shstrtab_offset, 0,
			section_offset, shstrtab_len, 0, 0, 0);
	fwrite(&x64->sh_shstrtab, sizeof(x64->sh_shstrtab), 1, elf->fp);

	// write user's section data
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		scf_loge("sh->name: %s, data: %p, len: %d\n", s->name->data, s->data, s->data_len);

		if (s->data && s->data_len > 0)
			fwrite(s->data, s->data_len, 1, elf->fp);
	}

	// write user's symbols data (symtab section)
	// entry index 0 in symtab is NOTYPE
	Elf64_Sym sym0 = {0};
	sym0.st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
	fwrite(&sym0, sizeof(sym0), 1, elf->fp);

	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		fwrite(&sym->sym, sizeof(sym->sym), 1, elf->fp);
	}

	// write strtab data (strtab section, symbol names of symtab)
	uint8_t c = 0;
	fwrite(&c, sizeof(c), 1, elf->fp);
	for (i = 0; i < x64->symbols->size; i++) {
		scf_elf_x64_sym_t* sym = x64->symbols->data[i];

		if (sym->name)
			fwrite(sym->name->data, sym->name->len + 1, 1, elf->fp);
	}

	// write shstrtab data (shstrtab section, section names of all sections)
	fwrite(&c, sizeof(c), 1, elf->fp);
	for (i = 0; i < x64->sections->size; i++) {
		scf_elf_x64_section_t* s = x64->sections->data[i];

		fwrite(s->name->data, s->name->len + 1, 1, elf->fp);
	}

	char* str = ".symtab\0.strtab\0.shstrtab\0";
	int str_len = strlen(".symtab") + strlen(".strtab") + strlen(".shstrtab") + 3;
	fwrite(str, str_len, 1, elf->fp);
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

static int _x64_elf_find_sym(scf_elf_x64_sym_t** psym, Elf64_Rela* rela, scf_vector_t* symbols)
{
	scf_elf_x64_sym_t*  sym;
	scf_elf_x64_sym_t*  sym2;

	int sym_idx = ELF64_R_SYM(rela->r_info);
	int j;

	assert(sym_idx >= 1);
	assert(sym_idx -  1 < symbols->size);

	sym = symbols->data[sym_idx - 1];

	if (0 == sym->sym.st_shndx) {

		int n = 0;

		for (j   = 0; j < symbols->size; j++) {
			sym2 =        symbols->data[j];

			if (0 == sym2->sym.st_shndx)
				continue;

			if (STB_LOCAL == ELF64_ST_BIND(sym2->sym.st_info))
				continue;

			if (!strcmp(sym2->name->data, sym->name->data)) {
				sym     = sym2;
				sym_idx = j + 1;
				n++;
			}
		}

		if (0 == n) {
			scf_loge("global symbol: %s not found\n", sym->name->data);
			return -1;

		} else if (n > 1) {
			scf_loge("tow global symbol: %s\n", sym->name->data);
			return -1;
		}
	} else if (ELF64_ST_TYPE(sym->sym.st_info) == STT_SECTION) {

		for (j   = symbols->size - 1; j >= 0; j--) {
			sym2 = symbols->data[j];

			if (ELF64_ST_TYPE(sym2->sym.st_info) != STT_SECTION)
				continue;

			if (sym2->sym.st_shndx == sym->sym.st_shndx) {
				sym     = sym2;
				sym_idx = j + 1;
				break;
			}
		}

		assert(j >= 0);
	}

	*psym = sym;
	return sym_idx;
}

static int _x64_elf_link_cs(scf_elf_x64_t* x64, scf_elf_x64_section_t* s, scf_elf_x64_section_t* rs, uint64_t cs_base)
{
	scf_elf_x64_sym_t*  sym;
	Elf64_Rela*         rela;

	assert(rs->data_len % sizeof(Elf64_Rela) == 0);

	int i;
	for (i   = 0; i < rs->data_len; i += sizeof(Elf64_Rela)) {

		rela = (Elf64_Rela* )(rs->data + i);
		sym  = NULL;

		int sym_idx = ELF64_R_SYM(rela->r_info);

		assert(sym_idx >= 1);
		assert(sym_idx -  1 < x64->symbols->size);

		sym = x64->symbols->data[sym_idx - 1];
		if (sym->dyn_flag) {
			scf_loge("sym '%s' in dynamic so\n", sym->name->data);
			continue;
		}

		int j = _x64_elf_find_sym(&sym, rela, x64->symbols);
		if (j < 0)
			return -1;

		assert(ELF64_R_TYPE(rela->r_info) == R_X86_64_PC32);

		int32_t offset = sym->sym.st_value - (cs_base + rela->r_offset) + rela->r_addend;

		rela->r_info = ELF64_R_INFO(j, ELF64_R_TYPE(rela->r_info));

		memcpy(s->data + rela->r_offset, &offset, sizeof(offset));
	}

	return 0;
}

static int _x64_elf_link_ds(scf_elf_x64_t* x64, scf_elf_x64_section_t* s, scf_elf_x64_section_t* rs)
{
	scf_elf_x64_sym_t*  sym;
	Elf64_Rela*         rela;

	assert(rs->data_len % sizeof(Elf64_Rela) == 0);

	int i;
	for (i   = 0; i < rs->data_len; i += sizeof(Elf64_Rela)) {

		rela = (Elf64_Rela* )(rs->data + i);
		sym  = NULL;

		int j = _x64_elf_find_sym(&sym, rela, x64->symbols);
		if (j < 0)
			return -1;

		assert(ELF64_R_TYPE(rela->r_info) == R_X86_64_64
			|| ELF64_R_TYPE(rela->r_info) == R_X86_64_32);

		uint64_t offset = sym->sym.st_value + rela->r_addend;

		rela->r_info    = ELF64_R_INFO(j, ELF64_R_TYPE(rela->r_info));

		switch (ELF64_R_TYPE(rela->r_info)) {

			case R_X86_64_64:
				memcpy(s->data + rela->r_offset, &offset, 8);
				break;

			case R_X86_64_32:
				memcpy(s->data + rela->r_offset, &offset, 4);
				break;
			default:
				assert(0);
				break;
		};
	}

	return 0;
}

static int _x64_elf_link_debug(scf_elf_x64_t* x64, scf_elf_x64_section_t* s, scf_elf_x64_section_t* rs)
{
	scf_elf_x64_sym_t*  sym;
	scf_elf_x64_sym_t*  sym2;
	Elf64_Rela*         rela;

	assert(rs->data_len % sizeof(Elf64_Rela) == 0);

	int i;
	for (i   = 0; i < rs->data_len; i += sizeof(Elf64_Rela)) {

		rela = (Elf64_Rela* )(rs->data + i);
		sym  = NULL;

		int j = _x64_elf_find_sym(&sym, rela, x64->symbols);
		if (j < 0)
			return -1;

		uint64_t offset = sym->sym.st_value + rela->r_addend;

		if (!strncmp(sym->name->data, ".debug_", 7)) {

			int k  = ELF64_R_SYM(rela->r_info);

			sym2   = x64->symbols->data[k - 1];

			offset = sym2->sym.st_value + rela->r_addend;
			rela->r_addend = offset;

		} else if (!strcmp(sym->name->data, ".text")) {

			int k  = ELF64_R_SYM(rela->r_info);

			sym2   = x64->symbols->data[k - 1];

			offset = sym2->sym.st_value;
			rela->r_addend = sym2->sym.st_value - sym->sym.st_value;
		}

		rela->r_info = ELF64_R_INFO(j, ELF64_R_TYPE(rela->r_info));

		switch (ELF64_R_TYPE(rela->r_info)) {

			case R_X86_64_64:
				memcpy(s->data + rela->r_offset, &offset, 8);
				break;

			case R_X86_64_32:
				memcpy(s->data + rela->r_offset, &offset, 4);
				break;
			default:
				assert(0);
				break;
		};
	}

	return 0;
}

static int _x64_elf_link_sections(scf_elf_x64_t* x64, uint32_t cs_index, uint32_t ds_index)
{
	scf_elf_x64_section_t* s;
	scf_elf_x64_section_t* rs;

	int i;
	for (i = 0; i < x64->sections->size; i++) {
		rs =        x64->sections->data[i];

		if (SHT_RELA != rs->sh.sh_type)
			continue;

		assert(rs->sh.sh_info < x64->sections->size);

		if (cs_index == rs->sh.sh_info
				|| ds_index == rs->sh.sh_info)
			continue;

		if (!strcmp(rs->name->data, ".rela.plt"))
			continue;

		s = x64->sections->data[rs->sh.sh_info - 1];

		scf_loge("s: %s, rs: %s, rs->sh.sh_info: %u\n", s->name->data, rs->name->data, rs->sh.sh_info);

		assert(!strcmp(s->name->data, rs->name->data + 5));

		if (_x64_elf_link_debug(x64, s, rs) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	return 0;
}

static void _x64_elf_process_syms(scf_elf_x64_t* x64, uint32_t cs_index)
{
	scf_elf_x64_section_t* s;
	scf_elf_x64_sym_t*     sym;
	Elf64_Rela*            rela;

	int i;
	int j;
	int k;
	int shndx = 20;
#if 1
	for (i  = x64->symbols->size - 1; i >= 0; i--) {
		sym = x64->symbols->data[i];

		if (STT_SECTION == ELF64_ST_TYPE(sym->sym.st_info)) {
			if (shndx > cs_index) {

				shndx = sym->sym.st_shndx;

				assert(sym->sym.st_shndx - 1 < x64->sections->size);

				sym->section = x64->sections->data[sym->sym.st_shndx - 1];
				continue;
			}
		} else if (0 != sym->sym.st_shndx) {

			if (sym->sym.st_shndx - 1 < x64->sections->size)
				sym->section = x64->sections->data[sym->sym.st_shndx - 1];
			continue;
		}

		assert(0 == scf_vector_del(x64->symbols, sym));

		scf_string_free(sym->name);
		free(sym);
	}
#endif
	qsort(x64->symbols->data, x64->symbols->size, sizeof(void*), _sym_cmp);

	for (j = 0; j < x64->sections->size; j++) {
		s  =        x64->sections->data[j];

		if (SHT_RELA != s->sh.sh_type)
			continue;

		if (!strcmp(s->name->data, ".rela.plt"))
			continue;

		assert(s->data_len % sizeof(Elf64_Rela) == 0);

		int sym_idx;
		for (k = 0; k < s->data_len; k += sizeof(Elf64_Rela)) {

			rela    = (Elf64_Rela*)(s->data + k);

			sym_idx = ELF64_R_SYM(rela->r_info);

			for (i  = 0; i < x64->symbols->size; i++) {
				sym =        x64->symbols->data[i];

				if (sym_idx == sym->index)
					break;
			}

			assert(i < x64->symbols->size);

			rela->r_info = ELF64_R_INFO(i + 1, ELF64_R_TYPE(rela->r_info));
		}
	}
}

static int _x64_elf_write_exec(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64      = elf->priv;
	int 		   nb_phdrs = 3;

	if (x64->dynsyms && x64->dynsyms->size) {
		__x64_elf_add_dyn(x64);
		nb_phdrs = 6;
	}

	int 		   nb_sections      = 1 + x64->sections->size + 1 + 1 + 1;
	uint64_t	   shstrtab_offset  = 1;
	uint64_t	   strtab_offset    = 1;
	uint64_t	   dynstr_offset    = 1;
	Elf64_Off      phdr_offset      = sizeof(x64->eh) + sizeof(Elf64_Shdr) * nb_sections;
	Elf64_Off      section_offset   = phdr_offset     + sizeof(Elf64_Phdr) * nb_phdrs;

	scf_elf_x64_section_t* s;
	scf_elf_x64_section_t* cs    = NULL;
	scf_elf_x64_section_t* ros   = NULL;
	scf_elf_x64_section_t* ds    = NULL;
	scf_elf_x64_section_t* crela = NULL;
	scf_elf_x64_section_t* drela = NULL;
	scf_elf_x64_sym_t*     sym;

	int i;
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (!strcmp(".text", s->name->data)) {

			assert(s->data_len > 0);
			assert(!cs);
			cs = s;

		} else if (!strcmp(".rodata", s->name->data)) {

			assert(s->data_len >= 0);
			assert(!ros);
			ros = s;

		} else if (!strcmp(".data", s->name->data)) {

			assert(s->data_len >= 0);
			assert(!ds);
			ds = s;

		} else if (!strcmp(".rela.text", s->name->data)) {

			assert(!crela);
			crela = s;

		} else if (!strcmp(".rela.data", s->name->data)) {

			assert(!drela);
			drela = s;
		}

		s->offset        = section_offset;
		section_offset  += s->data_len;
	}
	assert(crela);

	uint64_t cs_align  = (cs ->offset + cs ->data_len + 0x200000 - 1) >> 21 << 21;
	uint64_t ro_align  = (ros->offset + ros->data_len + 0x200000 - 1) >> 21 << 21;

	uint64_t rx_base   = 0x400000;
	uint64_t r_base    = 0x400000 + cs_align;
	uint64_t rw_base   = 0x400000 + cs_align + ro_align;

	uint64_t cs_base   = cs->offset  + rx_base;
	uint64_t ro_base   = ros->offset + r_base;
	uint64_t ds_base   = ds->offset  + rw_base;
	uint64_t _start    =  0;

	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		uint32_t shndx = sym->sym.st_shndx;

		if (shndx == cs->index)
			sym->sym.st_value += cs_base;

		else if (shndx == ros->index)
			sym->sym.st_value += ro_base;

		else if (shndx == ds->index)
			sym->sym.st_value += ds_base;

		scf_logd("sym: %s, %#lx, st_shndx: %d\n", sym->name->data, sym->sym.st_value, sym->sym.st_shndx);
	}

	int ret = _x64_elf_link_cs(x64, cs, crela, cs_base);
	if (ret < 0)
		return ret;

	if (drela) {
		ret = _x64_elf_link_ds(x64, ds, drela);
		if (ret < 0)
			return ret;
	}

	ret = _x64_elf_link_sections(x64, cs->index, ds->index);
	if (ret < 0)
		return ret;

	_x64_elf_process_syms(x64, cs->index);

	cs ->sh.sh_addr = cs_base;
	ds ->sh.sh_addr = ds_base;
	ros->sh.sh_addr = ro_base;


	if (6 == nb_phdrs) {
		__x64_elf_post_dyn(x64, rx_base, rw_base, cs);
	}

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
	_x64_elf_header_fill(&x64->eh, ET_EXEC, _start, phdr_offset, nb_phdrs, nb_sections, nb_sections - 1);
	fwrite(&x64->eh, sizeof(x64->eh), 1, elf->fp);

	// write null section header
	fwrite(&x64->sh_null, sizeof(x64->sh_null), 1, elf->fp);

	// write user's section header
	section_offset   = phdr_offset + sizeof(Elf64_Phdr) * nb_phdrs;

	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (SHT_RELA == s->sh.sh_type && 0 == s->sh.sh_link)
			s->sh.sh_link = nb_sections - 3;

		_x64_elf_section_header_fill(&s->sh, shstrtab_offset, s->sh.sh_addr,
				section_offset, s->data_len,
				s->sh.sh_link,  s->sh.sh_info, s->sh.sh_entsize);

		if (SHT_STRTAB != s->sh.sh_type)
			s->sh.sh_addralign = 8;

		section_offset  += s->data_len;
		shstrtab_offset += s->name->len + 1;

		fwrite(&s->sh, sizeof(s->sh), 1, elf->fp);
	}

	// set user's symbols' name
	int  nb_local_syms = 1;

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

	// write symtab section header
	_x64_elf_section_header_fill(&x64->sh_symtab, shstrtab_offset, 0,
			section_offset, (x64->symbols->size + 1) * sizeof(Elf64_Sym),
			nb_sections - 2, nb_local_syms, sizeof(Elf64_Sym));

	fwrite(&x64->sh_symtab, sizeof(x64->sh_symtab), 1, elf->fp);

	section_offset   += (x64->symbols->size + 1) * sizeof(Elf64_Sym);
	shstrtab_offset  += strlen(".symtab") + 1;

	// write strtab section header
	_x64_elf_section_header_fill(&x64->sh_strtab, shstrtab_offset, 0,
			section_offset, strtab_offset,
			0, 0, 0);
	fwrite(&x64->sh_strtab, sizeof(x64->sh_strtab), 1, elf->fp);
	section_offset   += strtab_offset;
	shstrtab_offset  += strlen(".strtab") + 1;

	// write shstrtab section header
	uint64_t shstrtab_len = shstrtab_offset + strlen(".shstrtab") + 1;
	_x64_elf_section_header_fill(&x64->sh_shstrtab, shstrtab_offset, 0,
			section_offset, shstrtab_len, 0, 0, 0);
	fwrite(&x64->sh_shstrtab, sizeof(x64->sh_shstrtab), 1, elf->fp);

#if 1
	if (6 == nb_phdrs) {
		__x64_elf_write_phdr(elf, rx_base, phdr_offset, nb_phdrs);

		__x64_elf_write_interp(elf, rx_base, x64->interp->offset, x64->interp->data_len);
	}

	__x64_elf_write_text  (elf, rx_base, 0,           cs->offset + cs->data_len);
	__x64_elf_write_rodata(elf, r_base,  ros->offset, ros->data_len);

	if (6 == nb_phdrs) {
		__x64_elf_write_data(elf, rw_base, x64->dynamic->offset,
				x64->dynamic->data_len + x64->got_plt->data_len + ds->data_len);

		__x64_elf_write_dynamic(elf, rw_base, x64->dynamic->offset, x64->dynamic->data_len);
	} else {
		__x64_elf_write_data(elf, rw_base, ds->offset, ds->data_len);
	}
#endif

	// write user's section data
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		if (s->data && s->data_len > 0)
			fwrite(s->data, s->data_len, 1, elf->fp);
	}

	// write user's symbols data (symtab section)
	// entry index 0 in symtab is NOTYPE
	Elf64_Sym sym0 = {0};
	sym0.st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);

	fwrite(&sym0, sizeof(sym0), 1, elf->fp);
	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		fwrite(&sym->sym, sizeof(sym->sym), 1, elf->fp);
	}

	// write strtab data (strtab section, symbol names of symtab)
	uint8_t c = 0;
	fwrite(&c, sizeof(c), 1, elf->fp);
	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		if (sym->name)
			fwrite(sym->name->data, sym->name->len + 1, 1, elf->fp);
	}

	// write shstrtab data (shstrtab section, section names of all sections)
	fwrite(&c, sizeof(c), 1, elf->fp);
	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		fwrite(s->name->data, s->name->len + 1, 1, elf->fp);
	}

	char* str   = ".symtab\0.strtab\0.shstrtab\0";
	int str_len = strlen(".symtab") + strlen(".strtab") + strlen(".shstrtab") + 3;
	fwrite(str, str_len, 1, elf->fp);
	return 0;
}

scf_elf_ops_t	elf_ops_x64 =
{
	.machine	      = "x64",

	.open		      = _x64_elf_open,
	.close		      = _x64_elf_close,

	.add_sym	      = _x64_elf_add_sym,
	.add_section      = _x64_elf_add_section,

	.add_rela_section = _x64_elf_add_rela_section,

	.add_dyn_need     = _x64_elf_add_dyn_need,
	.add_dyn_rela     = _x64_elf_add_dyn_rela,

	.read_syms        = _x64_elf_read_syms,
	.read_relas       = _x64_elf_read_relas,
	.read_section     = _x64_elf_read_section,

	.write_rel	      = _x64_elf_write_rel,
	.write_exec       = _x64_elf_write_exec,
};

