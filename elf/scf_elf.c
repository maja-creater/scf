#include"scf_elf.h"

extern scf_elf_ops_t	elf_ops_x64;

scf_elf_ops_t*			elf_ops_array[] =
{
	&elf_ops_x64,

	NULL,
};

int scf_elf_open(scf_elf_context_t** pelf, const char* machine, const char* path, const char* mode)
{
	scf_elf_context_t* elf = calloc(1, sizeof(scf_elf_context_t));
	assert(elf);

	int i;
	for (i = 0; elf_ops_array[i]; i++) {
		if (!strcmp(elf_ops_array[i]->machine, machine)) {
			elf->ops = elf_ops_array[i];
			break;
		}
	}

	if (!elf->ops) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (elf->ops->open && elf->ops->open(elf, path, mode) == 0) {
		*pelf = elf;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);

	free(elf);
	elf = NULL;
	return -1;
}

int scf_elf_close(scf_elf_context_t* elf)
{
	if (elf) {
		if (elf->ops && elf->ops->close)
			elf->ops->close(elf);

		free(elf);
		elf = NULL;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_add_sym(scf_elf_context_t* elf, const scf_elf_sym_t* sym)
{
	if (elf && sym) {

		if (elf->ops && elf->ops->add_sym)
			return elf->ops->add_sym(elf, sym);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section)
{
	if (elf && section) {

		if (elf->ops && elf->ops->add_section)
			return elf->ops->add_section(elf, section);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_add_rela(scf_elf_context_t* elf, const scf_elf_rela_t* rela)
{
	if (elf && rela) {

		if (elf->ops && elf->ops->add_rela)
			return elf->ops->add_rela(elf, rela);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_read_section(scf_elf_context_t* elf, scf_elf_section_t** psection, const char* name)
{
	if (elf && psection && name) {

		if (elf->ops && elf->ops->read_section)
			return elf->ops->read_section(elf, psection, name);
	}

	scf_loge("\n");
	return -1;
}

int scf_elf_write_rel(scf_elf_context_t* elf, scf_list_t* code_list_head)
{
	if (elf) {

		if (elf->ops && elf->ops->write_rel)
			return elf->ops->write_rel(elf, code_list_head);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_write_dyn(scf_elf_context_t* elf, scf_list_t* code_list_head)
{
	if (elf && code_list_head) {

		if (elf->ops && elf->ops->write_rel)
			return elf->ops->write_dyn(elf, code_list_head);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

int scf_elf_write_exec(scf_elf_context_t* elf, scf_list_t* code_list_head)
{
	if (elf && code_list_head) {

		if (elf->ops && elf->ops->write_rel)
			return elf->ops->write_exec(elf, code_list_head);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

