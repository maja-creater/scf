#include"scf_elf.h"
#include"scf_string.h"

typedef struct {
	scf_elf_context_t* elf;

	int                cs_idx;
	int                ds_idx;

	int                abbrev_idx;
	int                info_idx;
	int                line_idx;
	int                str_idx;

	scf_string_t*      text;
	scf_string_t*      data;

	scf_string_t*      debug_abbrev;
	scf_string_t*      debug_info;
	scf_string_t*      debug_line;
	scf_string_t*      debug_str;

	scf_vector_t*      syms;

	scf_vector_t*      text_relas;
	scf_vector_t*      data_relas;

	scf_vector_t*      debug_line_relas;
	scf_vector_t*      debug_info_relas;

} scf_elf_file_t;


int scf_elf_file_open(scf_elf_file_t** pfile, const char* path, const char* mode)
{
	scf_elf_file_t* ef = calloc(1, sizeof(scf_elf_file_t));
	if (!ef)
		return -ENOMEM;

	ef->cs_idx     = -1;
	ef->ds_idx     = -1;

	ef->abbrev_idx = -1;
	ef->info_idx   = -1;
	ef->line_idx   = -1;
	ef->str_idx    = -1;

	int ret;

	ef->text = scf_string_alloc();
	if (!ef->text) {
		ret = -ENOMEM;
		goto text_error;
	}

	ef->data = scf_string_alloc();
	if (!ef->data) {
		ret = -ENOMEM;
		goto data_error;
	}

	ef->debug_abbrev = scf_string_alloc();
	if (!ef->debug_abbrev) {
		ret = -ENOMEM;
		goto debug_abbrev_error;
	}

	ef->debug_info = scf_string_alloc();
	if (!ef->debug_info) {
		ret = -ENOMEM;
		goto debug_info_error;
	}

	ef->debug_line = scf_string_alloc();
	if (!ef->debug_line) {
		ret = -ENOMEM;
		goto debug_line_error;
	}

	ef->debug_str = scf_string_alloc();
	if (!ef->debug_str) {
		ret = -ENOMEM;
		goto debug_str_error;
	}

	ef->syms = scf_vector_alloc();
	if (!ef->syms) {
		ret = -ENOMEM;
		goto syms_error;
	}

	ef->text_relas = scf_vector_alloc();
	if (!ef->text_relas) {
		ret = -ENOMEM;
		goto text_relas_error;
	}

	ef->data_relas = scf_vector_alloc();
	if (!ef->data_relas) {
		ret = -ENOMEM;
		goto data_relas_error;
	}

	ef->debug_line_relas = scf_vector_alloc();
	if (!ef->debug_line_relas) {
		ret = -ENOMEM;
		goto debug_line_relas_error;
	}

	ef->debug_info_relas = scf_vector_alloc();
	if (!ef->debug_info_relas) {
		ret = -ENOMEM;
		goto debug_info_relas_error;
	}

	ret = scf_elf_open(&ef->elf, "x64", path, mode);
	if (ret < 0)
		goto elf_open_error;

	*pfile = ef;
	return 0;

elf_open_error:
	scf_vector_free(ef->debug_info_relas);
debug_info_relas_error:
	scf_vector_free(ef->debug_line_relas);
debug_line_relas_error:
	scf_vector_free(ef->data_relas);
data_relas_error:
	scf_vector_free(ef->text_relas);
text_relas_error:
	scf_vector_free(ef->syms);
syms_error:
	scf_string_free(ef->debug_str);
debug_str_error:
	scf_string_free(ef->debug_line);
debug_line_error:
	scf_string_free(ef->debug_info);
debug_info_error:
	scf_string_free(ef->debug_abbrev);
debug_abbrev_error:
	scf_string_free(ef->data);
data_error:
	scf_string_free(ef->text);
text_error:
	free(ef);
	return ret;
}

int scf_elf_file_close(scf_elf_file_t* ef, void (*rela_free)(void*))
{
	scf_elf_close(ef->elf);

	scf_string_free(ef->text);
	scf_string_free(ef->data);

	scf_string_free(ef->debug_abbrev);
	scf_string_free(ef->debug_info);
	scf_string_free(ef->debug_line);
	scf_string_free(ef->debug_str);

	scf_vector_clear(ef->text_relas,       ( void (*)(void*) ) rela_free);
	scf_vector_clear(ef->data_relas,       ( void (*)(void*) ) rela_free);
	scf_vector_clear(ef->debug_line_relas, ( void (*)(void*) ) rela_free);

	scf_vector_free (ef->text_relas);
	scf_vector_free (ef->data_relas);
	scf_vector_free (ef->debug_line_relas);

	scf_vector_clear(ef->syms, ( void (*)(void*) ) free);
	scf_vector_free (ef->syms);

	free(ef);
	return 0;
}

int scf_elf_file_read(scf_elf_file_t* ef)
{
#define ELF_READ_SECTION(sname, idx) \
	do {\
		scf_elf_section_t* s = NULL; \
		\
		int ret = scf_elf_read_section(ef->elf, &s, "."#sname); \
		if (ret < 0) { \
			if (-404 != ret) { \
				scf_loge("\n"); \
				return ret; \
			} \
		} else { \
			if (s->data_len > 0) \
				ret = scf_string_cat_cstr_len(ef->sname, s->data, s->data_len); \
			\
			(idx) = s->index; \
			free(s);  \
			s = NULL; \
			if (ret < 0) { \
				scf_loge("\n"); \
				return ret; \
			} \
		} \
	} while (0)

	ELF_READ_SECTION(text, ef->cs_idx);
	ELF_READ_SECTION(data, ef->ds_idx);

	ELF_READ_SECTION(debug_abbrev, ef->abbrev_idx);
	ELF_READ_SECTION(debug_info,   ef->info_idx);
	ELF_READ_SECTION(debug_line,   ef->line_idx);
	ELF_READ_SECTION(debug_str,    ef->str_idx);

	int ret = scf_elf_read_syms(ef->elf, ef->syms);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

#define ELF_READ_RELAS(sname) \
	do { \
		int ret = scf_elf_read_relas(ef->elf, ef->sname##_relas, ".rela."#sname); \
		if (ret < 0 && ret != -404) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while(0);

	ELF_READ_RELAS(text);
	ELF_READ_RELAS(data);
	ELF_READ_RELAS(debug_info);
	ELF_READ_RELAS(debug_line);

	return 0;
}

static int merge_relas(scf_vector_t* dst, scf_vector_t* src, size_t offset, int nb_syms)
{
	scf_elf_rela_t* rela;
	scf_elf_rela_t* rela2;

	int  j;
	for (j    = 0; j < src->size; j++) {
		rela2 =        src->data[j];

		rela2->r_offset += offset;
		rela2->r_info    = ELF64_R_INFO(ELF64_R_SYM(rela2->r_info) + nb_syms, ELF64_R_TYPE(rela2->r_info));

		rela = calloc(1, sizeof(scf_elf_rela_t));
		if (!rela)
			return -ENOMEM;

		rela->name = strdup(rela2->name);
		if (!rela->name) {
			free(rela);
			return -ENOMEM;
		}

		if (scf_vector_add(dst, rela) < 0) {
			free(rela->name);
			free(rela);
			return -ENOMEM;
		}

		rela->r_offset = rela2->r_offset;
		rela->r_info   = rela2->r_info;
		rela->r_addend = rela2->r_addend;
	}

	return 0;
}

static int merge_objs(scf_elf_file_t* exec, char* inputs[], int nb_inputs)
{
	int nb_syms = 0;
	int i;
	for (i = 0; i < nb_inputs; i++) {

		scf_elf_sym_t*  sym;
		scf_elf_sym_t*  sym2;

		scf_elf_file_t* obj = NULL;

		if (scf_elf_file_open(&obj, inputs[i], "rb") < 0) {
			scf_loge("inputs[%d]: %s\n", i, inputs[i]);
			return -1;
		}

		if (scf_elf_file_read(obj) < 0) {
			scf_loge("\n");
			return -1;
		}

#define MERGE_RELAS(dst, src, offset) \
		do { \
			int ret = merge_relas(dst, src, offset, nb_syms); \
			if (ret < 0) { \
				scf_loge("\n"); \
				return ret; \
			} \
		} while (0)

		MERGE_RELAS(exec->text_relas,       obj->text_relas,       exec->text->len);
		MERGE_RELAS(exec->data_relas,       obj->data_relas,       exec->data->len);
		MERGE_RELAS(exec->debug_line_relas, obj->debug_line_relas, exec->debug_line->len);
		MERGE_RELAS(exec->debug_info_relas, obj->debug_info_relas, exec->debug_info->len);

		int  j;
		for (j   = 0; j < obj->syms->size; j++) {
			sym2 =        obj->syms->data[j];

			if (obj->cs_idx == sym2->st_shndx) {

				sym2->st_value  += exec->text->len;
				sym2->st_shndx  =  1;

			} else if (obj->ds_idx == sym2->st_shndx) {

				sym2->st_value  += exec->data->len;
				sym2->st_shndx  =  2;

			} else if (obj->abbrev_idx == sym2->st_shndx) {

				sym2->st_value  += exec->debug_abbrev->len;
				sym2->st_shndx  =  3;

			} else if (obj->info_idx == sym2->st_shndx) {

				sym2->st_value  += exec->debug_info->len;
				sym2->st_shndx  =  4;

			} else if (obj->line_idx == sym2->st_shndx) {

				sym2->st_value  += exec->debug_line->len;
				sym2->st_shndx  =  5;

			} else if (obj->str_idx == sym2->st_shndx) {

				sym2->st_value  += exec->debug_str->len;
				sym2->st_shndx  =  6;
			} else
				scf_logd("sym2->st_shndx: %d, cs: %d, ds: %d, input: %s\n", sym2->st_shndx, obj->cs_idx, obj->ds_idx, inputs[i]);

			int ret = scf_elf_add_sym(exec->elf, sym2);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			nb_syms++;
		}

#define MERGE_BIN(dst, src) \
		do { \
			if (src->len > 0) { \
				int ret = scf_string_cat(dst, src); \
				if (ret < 0) { \
					scf_loge("\n"); \
					return ret; \
				} \
			} \
		} while (0)

		MERGE_BIN(exec->text,         obj->text);
		MERGE_BIN(exec->data,         obj->data);

		MERGE_BIN(exec->debug_abbrev, obj->debug_abbrev);
		MERGE_BIN(exec->debug_info,   obj->debug_info);
		MERGE_BIN(exec->debug_line,   obj->debug_line);
		MERGE_BIN(exec->debug_str,    obj->debug_str);

		scf_elf_file_close(obj, free);
		obj = NULL;
	}

	return 0;
}

int main()
{
	scf_elf_file_t* exec = NULL;

	int ret;

	char* inputs[] = {
		"./_start.o",
		"./1.elf",
		"./2.elf",
	};

	ret = scf_elf_file_open(&exec, "./1.out", "wb");
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = merge_objs(exec, inputs, sizeof(inputs) / sizeof(inputs[0]));
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	uint16_t shndx = 1;
	size_t   bytes = 0;

#define ADD_SECTION(sname, flags, align, value) \
	do { \
		scf_elf_section_t s   = {0}; \
		scf_elf_sym_t     sym = {0}; \
		\
		s.name                = "."#sname; \
		s.sh_type             = SHT_PROGBITS; \
		s.sh_flags            = flags; \
		s.sh_addralign        = align; \
		s.data                = exec->sname->data; \
		s.data_len            = exec->sname->len; \
		\
		ret = scf_elf_add_section(exec->elf, &s); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
		\
		sym.name      = "."#sname;   \
		sym.st_size   = s.data_len; \
		sym.st_value  = value;       \
		sym.st_shndx  = shndx++;     \
		sym.st_info   = ELF64_ST_INFO(STB_LOCAL, STT_SECTION); \
		bytes        += s.data_len; \
		\
		ret = scf_elf_add_sym(exec->elf, &sym); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while (0)

	ADD_SECTION(text, SHF_ALLOC | SHF_EXECINSTR, 1, 0);
	ADD_SECTION(data, SHF_ALLOC | SHF_WRITE,     8, 0);

	ADD_SECTION(debug_abbrev, 0, 8, bytes);
	ADD_SECTION(debug_info,   0, 8, bytes);
	ADD_SECTION(debug_line,   0, 8, bytes);
	ADD_SECTION(debug_str,    0, 8, bytes);

#define ADD_RELA_SECTION(sname, info) \
	do { \
		if (exec->sname##_relas->size > 0) { \
			\
			scf_elf_section_t s; \
			\
			s.name         = ".rela."#sname; \
			s.sh_type      = SHT_RELA; \
			s.sh_flags     = SHF_INFO_LINK; \
			s.sh_addralign = 8; \
			s.data         = NULL; \
			s.data_len     = 0; \
			s.sh_link      = 0; \
			s.sh_info      = info; \
			\
			ret = scf_elf_add_rela_section(exec->elf, &s, exec->sname##_relas); \
			if (ret < 0) { \
				scf_loge("\n"); \
				return ret; \
			} \
		} \
	} while(0)

	ADD_RELA_SECTION(text,       1);
	ADD_RELA_SECTION(data,       2);
	ADD_RELA_SECTION(debug_info, 4);
	ADD_RELA_SECTION(debug_line, 5);

	scf_elf_write_exec(exec->elf);
	scf_elf_file_close(exec, free);

	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

