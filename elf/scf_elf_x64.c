#include"scf_elf_x64.h"
#include"scf_elf_link.h"

uint32_t _x64_elf_hash(const uint8_t* p)
{
	uint32_t k = 0;
	uint32_t u = 0;

	while (*p) {
		k = (k << 4) + *p++;
		u = k & 0xf0000000;

		if (u)
			k ^= u >> 24;
		k &= ~u;
	}
	return k;
}

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

static int _x64_elf_add_interp(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".interp");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	char*  interp = "/lib/ld64.so.1";
	size_t len    = strlen(interp);

	s->data = malloc(len + 1);
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	memcpy(s->data, interp, len);
	s->data[len] = '\0';
	s->data_len  = len + 1;

	s->index     = 1;

	s->sh.sh_type   = SHT_PROGBITS;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_addralign = 1;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_gnu_version(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".gnu.version");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->data = calloc(x64->dynsyms->size, sizeof(Elf64_Versym));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = x64->dynsyms->size * sizeof(Elf64_Versym);

	s->index     = 1;

	s->sh.sh_type   = SHT_GNU_versym;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_addralign = 8;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_gnu_version_r(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".gnu.version_r");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->data = calloc(1, sizeof(Elf64_Verneed) + sizeof(Elf64_Vernaux));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = sizeof(Elf64_Verneed) + sizeof(Elf64_Vernaux);

	s->index     = 1;

	s->sh.sh_type   = SHT_GNU_verneed;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_addralign = 8;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_dynsym(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".dynsym");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->data = calloc(x64->dynsyms->size + 1, sizeof(Elf64_Sym));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = (x64->dynsyms->size + 1) * sizeof(Elf64_Sym);

	s->index     = 1;

	s->sh.sh_type   = SHT_DYNSYM;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_info   = 1;
	s->sh.sh_addralign = 8;
	s->sh.sh_entsize   = sizeof(Elf64_Sym);

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_dynstr(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".dynstr");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->index     = 1;

	s->sh.sh_type   = SHT_STRTAB;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_addralign = 1;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_dynamic(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".dynamic");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	int nb_tags = x64->dyn_needs->size + 11 + 1;

	s->data = calloc(nb_tags, sizeof(Elf64_Dyn));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = nb_tags * sizeof(Elf64_Dyn);

	s->index     = 1;

	s->sh.sh_type   = SHT_PROGBITS;
	s->sh.sh_flags  = SHF_ALLOC | SHF_WRITE;
	s->sh.sh_addralign = 8;
	s->sh.sh_entsize   = sizeof(Elf64_Dyn);

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_got_plt(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".got.plt");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->data = calloc(x64->dynsyms->size + 3, sizeof(void*));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = (x64->dynsyms->size + 3) * sizeof(void*);

	s->index     = 1;

	s->sh.sh_type   = SHT_PROGBITS;
	s->sh.sh_flags  = SHF_ALLOC | SHF_WRITE;
	s->sh.sh_addralign = 8;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_rela_plt(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".rela.plt");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	s->data = calloc(x64->dynsyms->size, sizeof(Elf64_Rela));
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}
	s->data_len  = x64->dynsyms->size * sizeof(Elf64_Rela);

	s->index     = 1;

	s->sh.sh_type   = SHT_RELA;
	s->sh.sh_flags  = SHF_ALLOC | SHF_INFO_LINK;
	s->sh.sh_addralign = 8;
	s->sh.sh_entsize   = sizeof(Elf64_Rela);

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _x64_elf_add_plt(scf_elf_x64_t* x64, scf_elf_x64_section_t** ps)
{
	scf_elf_x64_section_t* s;

	s = calloc(1, sizeof(scf_elf_x64_section_t));
	if (!s)
		return -ENOMEM;

	s->name = scf_string_cstr(".plt");
	if (!s->name) {
		free(s);
		return -ENOMEM;
	}

	uint8_t plt_lazy[16] = {
		0xff, 0x35, 0, 0, 0, 0, // pushq  (%rip)
		0xff, 0x25, 0, 0, 0, 0, // jmpq  *(%rip)
		0x0f, 0x1f, 0x40, 0     // nop
	};

	uint8_t plt[16] = {
		0xff, 0x25, 0, 0, 0, 0, // jmpq *(%rip)
		0x68, 0,    0, 0, 0,    // push $0
		0xe9, 0,    0, 0, 0,    // jmp
	};

	s->data = malloc(sizeof(plt_lazy) + sizeof(plt) * x64->dynsyms->size);
	if (!s->data) {
		scf_string_free(s->name);
		free(s);
		return -ENOMEM;
	}

	memcpy(s->data, plt_lazy, sizeof(plt_lazy));
	s->data_len = sizeof(plt);

	int jmp = -32;
	int i;
	for (i = 0; i < x64->dynsyms->size; i++) {
		memcpy(s->data + s->data_len, plt, sizeof(plt));

		s->data[s->data_len + 7    ] = i;
		s->data[s->data_len + 7 + 1] = i >> 8;
		s->data[s->data_len + 7 + 2] = i >> 16;
		s->data[s->data_len + 7 + 3] = i >> 24;

		s->data[s->data_len + 12    ] = jmp;
		s->data[s->data_len + 12 + 1] = jmp >> 8;
		s->data[s->data_len + 12 + 2] = jmp >> 16;
		s->data[s->data_len + 12 + 3] = jmp >> 24;

		jmp -= 16;

		s->data_len += sizeof(plt);
	}

	s->index     = 1;

	s->sh.sh_type   = SHT_PROGBITS;
	s->sh.sh_flags  = SHF_ALLOC;
	s->sh.sh_addralign = 1;

	int ret = scf_vector_add(x64->sections, s);
	if (ret < 0) {
		scf_string_free(s->name);
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*ps = s;
	return 0;
}

static int _section_cmp(const void* v0, const void* v1)
{
	const scf_elf_x64_section_t* s0 = *(const scf_elf_x64_section_t**)v0;
	const scf_elf_x64_section_t* s1 = *(const scf_elf_x64_section_t**)v1;

	if (s0->index < s1->index)
		return -1;
	else if (s0->index > s1->index)
		return 1;
	return 0;
}

static int _x64_elf_process_dyn(scf_elf_x64_t* x64)
{
	scf_elf_x64_section_t* s;
	scf_elf_x64_sym_t*     sym;
	Elf64_Rela*            rela;

	int i;
	for (i  = x64->symbols->size - 1; i >= 0; i--) {
		sym = x64->symbols->data[i];

		uint16_t shndx = sym->sym.st_shndx;

		if (STT_SECTION == ELF64_ST_TYPE(sym->sym.st_info)) {
			if (shndx > 0) {
				assert(shndx - 1 < x64->sections->size);
				sym->section = x64->sections->data[shndx - 1];
			}
		} else if (0 != shndx) {
			if (shndx - 1 < x64->sections->size)
				sym->section = x64->sections->data[shndx - 1];
		}
	}

	char* sh_names[] = {
		".interp",
		".dynsym",
		".dynstr",
//		".gnu.version",
		".gnu.version_r",
		".rela.plt",
		".plt",

		".text",
		".rodata",

		".dynamic",
		".got.plt",
		".data",
	};

	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		s->index = x64->sections->size + 1 + sizeof(sh_names) / sizeof(sh_names[0]);

		scf_logw("s: %s, link: %d, info: %d\n", s->name->data, s->sh.sh_link, s->sh.sh_info);

		if (s->sh.sh_link > 0) {
			assert(s->sh.sh_link - 1 < x64->sections->size);

			s->link = x64->sections->data[s->sh.sh_link - 1];
		}

		if (s->sh.sh_info > 0) {
			assert(s->sh.sh_info - 1 < x64->sections->size);

			s->info = x64->sections->data[s->sh.sh_info - 1];
		}
	}

	_x64_elf_add_interp(x64, &x64->interp);
	_x64_elf_add_dynsym(x64, &x64->dynsym);
	_x64_elf_add_dynstr(x64, &x64->dynstr);

//	_x64_elf_add_gnu_version  (x64, &x64->gnu_version);
	_x64_elf_add_gnu_version_r(x64, &x64->gnu_version_r);

	_x64_elf_add_rela_plt(x64, &x64->rela_plt);
	_x64_elf_add_plt(x64, &x64->plt);

	_x64_elf_add_dynamic(x64, &x64->dynamic);
	_x64_elf_add_got_plt(x64, &x64->got_plt);

	scf_string_t* str = scf_string_alloc();

	char c = '\0';
	scf_string_cat_cstr_len(str, &c, 1);

//	Elf64_Versym* versyms = (Elf64_Versym*)x64->gnu_version->data;
	Elf64_Sym*    syms    = (Elf64_Sym*   )x64->dynsym->data;
	Elf64_Sym     sym0    = {0};

	sym0.st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
	memcpy(&syms[0], &sym0, sizeof(Elf64_Sym));

	for (i = 0; i < x64->dynsyms->size; i++) {
		scf_elf_x64_sym_t* xsym = x64->dynsyms->data[i];

		memcpy(&syms[i + 1], &xsym->sym, sizeof(Elf64_Sym));

		syms[i + 1].st_name = str->len;

//		versyms[i] = VER_NDX_LOCAL;
//		versyms[i] = 2;

		scf_loge("i: %d, st_value: %#lx\n", i, syms[i + 1].st_value);

		scf_string_cat_cstr_len(str, xsym->name->data, xsym->name->len + 1);
	}

	Elf64_Verneed* verneeds = (Elf64_Verneed*) x64->gnu_version_r->data;
	Elf64_Vernaux* vernauxs = (Elf64_Vernaux*)(x64->gnu_version_r->data +sizeof(Elf64_Verneed));

	Elf64_Dyn* dyns = (Elf64_Dyn*)x64->dynamic->data;

	verneeds[0].vn_version = VER_NEED_CURRENT;
	verneeds[0].vn_file    = str->len;
	verneeds[0].vn_cnt     = 1;
	verneeds[0].vn_aux     = sizeof(Elf64_Verneed);
	verneeds[0].vn_next    = 0;

	scf_string_cat_cstr_len(str, "libc.so.6", strlen("libc.so.6") + 1);

	vernauxs[0].vna_hash   = _x64_elf_hash("GLIBC_2.4");
	vernauxs[0].vna_flags  = 0;
	vernauxs[0].vna_other  = 2;
	vernauxs[0].vna_name   = str->len;
	vernauxs[0].vna_next   = 0;

	scf_string_cat_cstr_len(str, "GLIBC_2.4", strlen("GLIBC_2.4") + 1);

	for (i = 0; i < x64->dyn_needs->size; i++) {
		scf_string_t* needed = x64->dyn_needs->data[i];

		dyns[i].d_tag = DT_NEEDED;
		dyns[i].d_un.d_val = str->len;

		scf_string_cat_cstr_len(str, needed->data, needed->len + 1);
	}

	dyns[i].d_tag     = DT_STRTAB;
	dyns[i + 1].d_tag = DT_SYMTAB;
	dyns[i + 2].d_tag = DT_STRSZ;
	dyns[i + 3].d_tag = DT_SYMENT;
	dyns[i + 4].d_tag = DT_PLTGOT;
	dyns[i + 5].d_tag = DT_PLTRELSZ;
	dyns[i + 6].d_tag = DT_PLTREL;
	dyns[i + 7].d_tag = DT_JMPREL;
	dyns[i + 8].d_tag = DT_VERNEED;
	dyns[i + 9].d_tag = DT_VERNEEDNUM;
//	dyns[i +10].d_tag = DT_VERSYM;
	dyns[i +10].d_tag = DT_NULL;

	dyns[i].d_un.d_ptr     = (uintptr_t)x64->dynstr;
	dyns[i + 1].d_un.d_ptr = (uintptr_t)x64->dynsym;
	dyns[i + 2].d_un.d_val = str->len;
	dyns[i + 3].d_un.d_val = sizeof(Elf64_Sym);
	dyns[i + 4].d_un.d_ptr = (uintptr_t)x64->got_plt;
	dyns[i + 5].d_un.d_ptr = sizeof(Elf64_Rela);
	dyns[i + 6].d_un.d_ptr = DT_RELA;
	dyns[i + 7].d_un.d_ptr = (uintptr_t)x64->rela_plt;
	dyns[i + 8].d_un.d_ptr = (uintptr_t)x64->gnu_version_r;
	dyns[i + 9].d_un.d_ptr = 1;
//	dyns[i +10].d_un.d_ptr = (uintptr_t)x64->gnu_version;
	dyns[i +10].d_un.d_ptr = 0;

	x64->dynstr->data     = str->data;
	x64->dynstr->data_len = str->len;

	str->data = NULL;
	str->len  = 0;
	str->capacity = 0;
	scf_string_free(str);
	str = NULL;

	x64->rela_plt->link = x64->dynsym;
	x64->rela_plt->info = x64->got_plt;
	x64->dynsym  ->link = x64->dynstr;

//	x64->gnu_version  ->link = x64->dynsym;
	x64->gnu_version_r->link = x64->dynstr;
	x64->gnu_version_r->info = x64->interp;


	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		int j;
		for (j = 0; j < sizeof(sh_names) / sizeof(sh_names[0]); j++) {
			if (!strcmp(s->name->data, sh_names[j]))
				break;
		}

		if (j < sizeof(sh_names) / sizeof(sh_names[0]))
			s->index = j + 1;

		scf_logd("i: %d, s: %s, index: %d\n", i, s->name->data, s->index);
	}

	qsort(x64->sections->data, x64->sections->size, sizeof(void*), _section_cmp);

	int j = sizeof(sh_names) / sizeof(sh_names[0]);

	for (i = j; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		s->index = i + 1;
	}

	for (i = 0; i < x64->sections->size; i++) {
		s  =        x64->sections->data[i];

		scf_loge("i: %d, s: %s, index: %d\n", i, s->name->data, s->index);

		if (s->link) {
			scf_logd("link: %s, index: %d\n", s->link->name->data, s->link->index);
			s->sh.sh_link = s->link->index;
		}

		if (s->info) {
			scf_logd("info: %s, index: %d\n", s->info->name->data, s->info->index);
			s->sh.sh_info = s->info->index;
		}
	}

#if 1
	for (i  = 0; i < x64->symbols->size; i++) {
		sym =        x64->symbols->data[i];

		if (sym->section) {
			scf_logd("sym: %s, index: %d->%d\n", sym->name->data, sym->sym.st_shndx, sym->section->index);
			sym->sym.st_shndx = sym->section->index;
		}
	}
#endif
	return 0;
}

static int _x64_elf_write_exec(scf_elf_context_t* elf)
{
	scf_elf_x64_t* x64              = elf->priv;

	_x64_elf_process_dyn(x64);

	int            nb_dyns          = 0;
	int 		   nb_sections      = 1 + x64->sections->size + 1 + 1 + 1;
	int 		   nb_phdrs         = 6;
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

	x64->gnu_version_r->sh.sh_addr = rx_base + x64->gnu_version_r->offset;

	x64->rela_plt->sh.sh_addr = rx_base + x64->rela_plt->offset;
	x64->dynamic->sh.sh_addr  = rw_base + x64->dynamic->offset;
	x64->got_plt->sh.sh_addr  = rw_base + x64->got_plt->offset;
	x64->interp->sh.sh_addr   = rx_base + x64->interp->offset;
	x64->plt->sh.sh_addr      = rx_base + x64->plt->offset;

	Elf64_Rela* rela_plt = (Elf64_Rela*)x64->rela_plt->data;
	Elf64_Sym*  dynsym   = (Elf64_Sym* )x64->dynsym->data;
	uint64_t*   got_plt  = (uint64_t*  )x64->got_plt->data;
	uint8_t*    plt      = (uint8_t*   )x64->plt->data;

	uint64_t   got_addr = x64->got_plt->sh.sh_addr + 8;
	uint64_t   plt_addr = x64->plt->sh.sh_addr;
	int32_t    offset   = got_addr - plt_addr - 6;

	got_plt[0] = x64->dynamic->sh.sh_addr;
	got_plt[1] = 0;
	got_plt[2] = 0;

	plt[2] = offset;
	plt[3] = offset >> 8;
	plt[4] = offset >> 16;
	plt[5] = offset >> 24;

	offset += 2;
	plt[8]  = offset;
	plt[9]  = offset >> 8;
	plt[10] = offset >> 16;
	plt[11] = offset >> 24;

	got_plt += 3;
	plt     += 16;

	got_addr += 16;
	plt_addr += 16;

	for (i = 0; i < x64->dynsyms->size; i++) {
		rela_plt[i].r_offset   = got_addr;
		rela_plt[i].r_addend   = 0;
		rela_plt[i].r_info     = ELF64_R_INFO(i + 1, R_X86_64_JUMP_SLOT);

		*got_plt = plt_addr + 6;

		offset = got_addr - plt_addr - 6;

		plt[2] = offset;
		plt[3] = offset >> 8;
		plt[4] = offset >> 16;
		plt[5] = offset >> 24;

		plt += 16;
		plt_addr += 16;
		got_addr += 8;
		got_plt++;
	}

	for (i = 0; i < x64->dyn_relas->size; i++) {
		Elf64_Rela* r = x64->dyn_relas->data[i];

		int sym_idx = ELF64_R_SYM(r->r_info);
		assert(sym_idx > 0);

		uint64_t plt_addr = x64->plt->sh.sh_addr + sym_idx * 16;

		int32_t offset = plt_addr - (cs_base + r->r_offset) + r->r_addend;

		memcpy(cs->data + r->r_offset, &offset, sizeof(offset));
	}

	Elf64_Dyn* dtags = (Elf64_Dyn*)x64->dynamic->data;

	for (i  = x64->dyn_needs->size; i < x64->dynamic->data_len / sizeof(Elf64_Dyn); i++) {

		scf_elf_x64_section_t* s = (scf_elf_x64_section_t*)dtags[i].d_un.d_ptr;

		switch (dtags[i].d_tag) {

			case DT_SYMTAB:
			case DT_STRTAB:
			case DT_JMPREL:
			case DT_VERNEED:
			case DT_VERSYM:
				dtags[i].d_un.d_ptr = s->offset + rx_base;
				s->sh.sh_addr       = s->offset + rx_base;
				break;

			case DT_PLTGOT:
				dtags[i].d_un.d_ptr = s->offset + rw_base;
				s->sh.sh_addr       = s->offset + rw_base;
				break;
			default:
				break;
		};
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
	// write program header
	Elf64_Phdr ph_phdr    = {0};
	Elf64_Phdr ph_interp  = {0};
	Elf64_Phdr ph_text    = {0};
	Elf64_Phdr ph_rodata  = {0};
	Elf64_Phdr ph_data    = {0};
	Elf64_Phdr ph_dynamic = {0};

	ph_phdr.p_type     = PT_PHDR;
	ph_phdr.p_flags    = PF_R;
	ph_phdr.p_offset   = phdr_offset;
	ph_phdr.p_vaddr    = 0x400000 + phdr_offset;
	ph_phdr.p_paddr    = ph_phdr.p_vaddr;
	ph_phdr.p_filesz   = sizeof(Elf64_Phdr) * nb_phdrs;
	ph_phdr.p_memsz    = ph_phdr.p_filesz;
	ph_phdr.p_align    = 0x8;

	fwrite(&ph_phdr,  sizeof(ph_phdr),  1, elf->fp);
#if 1
	ph_interp.p_type   = PT_INTERP;
	ph_interp.p_flags  = PF_R;
	ph_interp.p_offset = x64->interp->offset;
	ph_interp.p_vaddr  = rx_base + x64->interp->offset;
	ph_interp.p_paddr  = ph_interp.p_vaddr;
	ph_interp.p_filesz = x64->interp->data_len;
	ph_interp.p_memsz  = ph_interp.p_filesz;
	ph_interp.p_align  = 0x1;

	fwrite(&ph_interp, sizeof(ph_interp),  1, elf->fp);
#endif

#if 1
	ph_text.p_type     = PT_LOAD;
	ph_text.p_flags    = PF_R | PF_X;
	ph_text.p_offset   = 0;
	ph_text.p_vaddr    = 0x400000;
	ph_text.p_paddr    = ph_text.p_vaddr;
	ph_text.p_filesz   = cs->offset + cs->data_len;
	ph_text.p_memsz    = ph_text.p_filesz;
	ph_text.p_align    = 0x200000;

	fwrite(&ph_text,  sizeof(ph_text),  1, elf->fp);
#endif

#if 1
	ph_rodata.p_type   = PT_LOAD;
	ph_rodata.p_flags  = PF_R;
	ph_rodata.p_offset = ros->offset;
	ph_rodata.p_vaddr  = 0x400000 + cs_align + ros->offset;
	ph_rodata.p_paddr  = ph_rodata.p_vaddr;
	ph_rodata.p_filesz = ros->data_len;
	ph_rodata.p_memsz  = ph_rodata.p_filesz;
	ph_rodata.p_align  = 0x200000;

	fwrite(&ph_rodata,  sizeof(ph_rodata),  1, elf->fp);
#endif

#if 1
	ph_data.p_type     = PT_LOAD;
	ph_data.p_flags    = PF_R | PF_W;
	ph_data.p_offset   = x64->dynamic->offset;
	ph_data.p_vaddr    = 0x400000 + cs_align + ro_align + x64->dynamic->offset;
	ph_data.p_paddr    = ph_data.p_vaddr;
	ph_data.p_filesz   = x64->dynamic->data_len + x64->got_plt->data_len + ds->data_len;
	ph_data.p_memsz    = ph_data.p_filesz;
	ph_data.p_align    = 0x200000;

	fwrite(&ph_data,  sizeof(ph_data),  1, elf->fp);
#endif
#if 1
	ph_dynamic.p_type     = PT_DYNAMIC;
	ph_dynamic.p_flags    = PF_R | PF_W;
	ph_dynamic.p_offset   = x64->dynamic->offset;
	ph_dynamic.p_vaddr    = 0x400000 + cs_align + ro_align + x64->dynamic->offset;
	ph_dynamic.p_paddr    = ph_dynamic.p_vaddr;
	ph_dynamic.p_filesz   = x64->dynamic->data_len;
	ph_dynamic.p_memsz    = ph_dynamic.p_filesz;
	ph_dynamic.p_align    = 0x8;

	fwrite(&ph_dynamic,  sizeof(ph_data),  1, elf->fp);
#endif
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

