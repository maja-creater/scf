#include"scf_elf.h"
#include"scf_dwarf_def.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		scf_loge("\n");
		return -1;
	}

	scf_elf_context_t* elf = NULL;

	if (scf_elf_open(&elf, "x64", "/home/yu/my/1.o", "rb") < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_elf_section_t* s = NULL;

	int ret = scf_elf_read_section(elf, &s, argv[1]);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	printf("s->data_len: %d\n", s->data_len);
	int i;
	for (i = 0; i < s->data_len; i++) {
		if (i > 0 && i % 10 == 0)
			printf("\n");

		unsigned char c = s->data[i];
		printf("%#02x ", c);
	}
	printf("\n\n");

	scf_vector_t* abbrev_results = scf_vector_alloc();

	ret = scf_dwarf_abbrev_decode(abbrev_results, s->data, s->data_len);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_dwarf_abbrev_print(abbrev_results);

	scf_string_t* abbrev = scf_string_alloc();
	ret = scf_dwarf_abbrev_encode(abbrev_results, abbrev);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	if (abbrev->len != s->data_len) {
		scf_loge("\n");
		return -1;
	}

	for (i = 0; i < abbrev->len; i++) {
		if (i > 0 && i % 10 == 0)
			printf("\n");

		unsigned char c  = abbrev->data[i];
		unsigned char c2 = s->data[i];
		printf("%#02x ", c);

		if (c != c2) {
			scf_loge("\n");
			return -1;
		}
	}
	printf("\n\n");

	scf_elf_close(elf);
	elf = NULL;
	scf_logi("main ok\n");
	return 0;
}

