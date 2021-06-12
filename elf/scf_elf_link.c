#include"scf_elf.h"
#include"scf_string.h"

int main()
{
	scf_elf_context_t* obj    = NULL;
	scf_elf_context_t* exe    = NULL;

	scf_elf_rela_t*    rela;
	scf_elf_sym_t*     sym;
	scf_elf_sym_t*     sym2;

	scf_string_t*      text;
	scf_string_t*      data;


	text = scf_string_alloc();
	data = scf_string_alloc();
	assert(text);
	assert(data);

	int ret;

	char* inputs[] = {
		"./_start.o",
		"./1.elf"
	};

	ret = scf_elf_open(&exe, "x64", "./1.out", "wb");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	int nb_syms = 0;
	int i;
	for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {

		if (scf_elf_open(&obj, "x64", inputs[i], "rb") < 0) {
			scf_loge("\n");
			return -1;
		}

		scf_elf_section_t* cs2    = NULL;
		scf_elf_section_t* ds2    = NULL;

		scf_elf_rela_t*    rela2;

		scf_vector_t*      syms2;
		scf_vector_t*      relas2;

		syms2  = scf_vector_alloc();
		relas2 = scf_vector_alloc();

		assert(syms2);
		assert(relas2);

		ret = scf_elf_read_section(obj, &cs2, ".text");
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		ret = scf_elf_read_section(obj, &ds2, ".data");
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

			rela2->r_info    = ELF64_R_INFO(ELF64_R_SYM(rela2->r_info) + nb_syms, ELF64_R_TYPE(rela2->r_info));

			ret = scf_elf_add_rela(exe, rela2);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		for (j   = 0; j < syms2->size; j++) {
			sym2 =        syms2->data[j];

			if (cs2->index == sym2->st_shndx) {

				sym2->st_value  += text->len;
				sym2->st_shndx  =  1;

			} else if (ds2->index == sym2->st_shndx) {

				sym2->st_value  += data->len;
				sym2->st_shndx  =  2;
			} else
				scf_loge("sym2->st_shndx: %d, cs2->index: %d, input: %s\n", sym2->st_shndx, cs2->index, inputs[i]);

			ret = scf_elf_add_sym(exe, sym2);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			nb_syms++;
		}

		if (cs2->data_len > 0) {

			ret = scf_string_cat_cstr_len(text, cs2->data, cs2->data_len);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		if (ds2->data_len > 0) {

			ret = scf_string_cat_cstr_len(data, ds2->data, ds2->data_len);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		free(cs2);
		free(ds2);
		cs2 = NULL;
		ds2 = NULL;
	}

	scf_elf_section_t cs = {0};
	scf_elf_section_t ds = {0};

	cs.name              = ".text";
	cs.sh_type           = SHT_PROGBITS;
	cs.sh_flags          = SHF_ALLOC | SHF_EXECINSTR;
	cs.sh_addralign      = 1;
	cs.data              = text->data;
	cs.data_len          = text->len;

	ds.name              = ".data";
	ds.sh_type           = SHT_PROGBITS;
	ds.sh_flags          = SHF_ALLOC | SHF_WRITE;
	ds.sh_addralign      = 8;
	ds.data              = data->data;
	ds.data_len          = data->len;

	ret = scf_elf_add_section(exe, &cs);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = scf_elf_add_section(exe, &ds);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_elf_write_exec(exe);

	scf_elf_close(obj);
	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

