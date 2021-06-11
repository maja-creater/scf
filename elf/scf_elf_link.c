#include"scf_elf.h"
#include"scf_string.h"

int main()
{
	scf_elf_context_t* obj    = NULL;
	scf_elf_context_t* exe    = NULL;

	scf_elf_rela_t*    rela;
	scf_elf_sym_t*     sym;
	scf_elf_sym_t*     sym2;

	scf_vector_t*      syms;
	scf_vector_t*      relas;

	scf_string_t*      text;
	scf_string_t*      data;

	syms  = scf_vector_alloc();
	relas = scf_vector_alloc();
	assert(syms);
	assert(relas);

	text = scf_string_alloc();
	data = scf_string_alloc();
	assert(text);
	assert(data);

	int ret;

	char* inputs[] = {
		"./_start.o",
		"./1.elf"
	};

	int i;
	for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {

		if (scf_elf_open(&obj, "x64", inputs[i], "rb") < 0) {
			scf_loge("\n");
			return -1;
		}

		scf_elf_section_t* cs2    = NULL;
		scf_elf_section_t* ds2    = NULL;
		scf_vector_t*      syms2  = NULL;
		scf_vector_t*      relas2 = NULL;

		scf_elf_rela_t*    rela2;

		syms2  = scf_vector_alloc();
		relas2 = scf_vector_alloc();
		assert(syms2);
		assert(relas2);

		ret = scf_elf_read_section(obj, &cs2, ".text");
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		ret = scf_elf_read_syms(obj, syms2);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		ret = scf_elf_read_relas(obj, relas2);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		int j;
		for (j    = 0; j < relas2->size; j++) {
			rela2 =        relas2->data[j];

			rela2->r_offset += text->len;

			rela2->r_info    = ELF64_R_INFO(ELF64_R_SYM(rela2->r_info) + syms->size, ELF64_R_TYPE(rela2->r_info));

			assert(0 == scf_vector_add(relas, rela2));
		}

		for (j   = 0; j < syms2->size; j++) {
			sym2 =        syms2->data[j];

			if (cs2->index     == sym2->st_shndx)
				sym2->st_value += text->len;
			else {
				scf_loge("sym2->st_shndx: %d, cs2->index: %d, input: %s\n", sym2->st_shndx, cs2->index, inputs[i]);
			}

			assert(0 == scf_vector_add(syms, sym2));
		}

		if (cs2->data_len > 0) {

			ret = scf_string_cat_cstr_len(text, cs2->data, cs2->data_len);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		free(cs2);
		scf_vector_free(syms2);
		scf_vector_free(relas2);
		syms2  = NULL;
		relas2 = NULL;
		cs2    = NULL;
	}

	for (i   = 0; i < relas->size; i++) {
		rela =        relas->data[i];

		int sym_idx = ELF64_R_SYM(rela->r_info);

		assert(sym_idx >= 1);
		assert(sym_idx -  1 < syms->size);

		sym = syms->data[sym_idx - 1];

		if (0 == sym->st_shndx) {

			int n = 0;
			int j;

			for (j   = 0; j < syms->size; j++) {
				sym2 =        syms->data[j];

				if (0 == sym2->st_shndx)
					continue;

				if (STB_LOCAL == ELF64_ST_BIND(sym2->st_info))
					continue;

				if (!strcmp(sym2->name, sym->name)) {
					sym = sym2;
					n++;
				}
			}

			if (n > 1) {
				scf_loge("tow global symbol: %s\n", sym->name);
				return -1;
			}
		}

		int32_t offset = sym->st_value - rela->r_offset + rela->r_addend;

		scf_logw("rela %d: %s, r_offset: %#lx, sym: %s, st_value: %#lx, st_size: %ld, offset: %#x, st_shndx: %d\n",
				i, rela->name, rela->r_offset, sym->name, sym->st_value, sym->st_size, offset, sym->st_shndx);

		memcpy(text->data + rela->r_offset, &offset, sizeof(offset));
	}

	ret = scf_elf_open(&exe, "x64", "./1.out", "wb");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (i  = 0; i < syms->size; i++) {
		sym =        syms->data[i];

		if (STT_SECTION == ELF64_ST_TYPE(sym->st_info))
			continue;

		if (0 == sym->st_shndx)
			continue;

		scf_logw("sym %d: %s, st_shndx: %d\n", i, sym->name, sym->st_shndx);

		ret = scf_elf_add_sym(exe, sym);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_elf_section_t cs = {0};
	cs.name              = ".text";
	cs.sh_type           = SHT_PROGBITS;
	cs.sh_flags          = SHF_ALLOC | SHF_EXECINSTR;
	cs.sh_addralign      = 1;
	cs.data              = text->data;
	cs.data_len          = text->len;

	ret = scf_elf_add_section(exe, &cs);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_elf_write_exec(exe);

	scf_elf_close(obj);
	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

