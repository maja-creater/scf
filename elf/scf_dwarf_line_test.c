#include"scf_elf.h"
#include"scf_dwarf_def.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_elf_context_t* elf = NULL;

	if (scf_elf_open(&elf, "x64", "/home/yu/my/1.o", "rb") < 0) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	scf_elf_section_t* s = NULL;

	int ret = scf_elf_read_section(elf, &s, argv[1]);
	if (ret < 0) {
		printf("%s(),%d, error\n", __func__, __LINE__);
		return -1;
	}

	printf("s->data_len: %d\n", s->data_len);
	int i;
	for (i = 0; i < s->data_len; i++) {
		if (i > 0 && i % 4 == 0)
			printf("\n");

		unsigned char c = s->data[i];
		printf("%#02x ", c);
	}
	printf("\n\n");

	for (i = 0; i < s->data_len; i++) {
		unsigned char c = s->data[i];
		printf("%c", c);
	}
	printf("\n");

	scf_dwarf_line_machine_t* lm = scf_dwarf_line_machine_alloc();
	assert(lm);

	scf_vector_t* line_results = scf_vector_alloc();
	assert(line_results);
#if 1
	ret = scf_dwarf_line_decode(lm, line_results, s->data, s->data_len);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_dwarf_line_machine_print(lm);

#if 1
	for (i = 0; i < line_results->size; i++) {
		scf_dwarf_line_result_t* r = line_results->data[i];

		scf_loge("address: %#lx, line: %u, column: %u, is_stmt: %u, basic_block: %u, end_sequence: %u\n",
				r->address, r->line, r->column, r->is_stmt, r->basic_block, r->end_sequence);
	}
#endif
#endif

	scf_string_t* debug_line = scf_string_alloc();

	ret = scf_dwarf_line_encode(lm, line_results, debug_line);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

#if 1
	scf_vector_clear(line_results, (void (*)(void*))free);

	lm->prologue->total_length = debug_line->len - sizeof(scf_dwarf_uword_t);

	ret = scf_dwarf_line_decode(lm, line_results, debug_line->data, debug_line->len);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	for (i = 0; i < line_results->size; i++) {
		scf_dwarf_line_result_t* r = line_results->data[i];

		scf_loge("address: %#lx, line: %u, column: %u, is_stmt: %u, basic_block: %u, end_sequence: %u\n",
				r->address, r->line, r->column, r->is_stmt, r->basic_block, r->end_sequence);
	}
#endif

	scf_elf_close(elf);
	elf = NULL;
	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

