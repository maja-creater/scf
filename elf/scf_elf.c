#include"scf_elf.h"

extern scf_elf_ops_t	elf_ops_x64;

scf_elf_ops_t*			elf_ops_array[] =
{
	&elf_ops_x64,

	NULL,
};

void scf_elf_rela_free(scf_elf_rela_t* rela)
{
	if (rela) {
		if (rela->name)
			free(rela->name);

		free(rela);
	}
}

int scf_elf_open2(scf_elf_context_t* elf, const char* machine)
{
	if (!elf->fp) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; elf_ops_array[i]; i++) {
		if (!strcmp(elf_ops_array[i]->machine, machine)) {
			elf->ops = elf_ops_array[i];
			break;
		}
	}

	if (!elf->ops) {
		scf_loge("\n");
		return -1;
	}

	if (elf->ops->open && elf->ops->open(elf) == 0)
		return 0;

	scf_loge("\n");
	return -1;
}

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
		scf_loge("\n");
		return -1;
	}

	elf->fp = fopen(path, mode);
	if (!elf->fp) {
		scf_loge("\n");
		return -1;
	}

	if (elf->ops->open && elf->ops->open(elf) == 0) {
		*pelf = elf;
		return 0;
	}

	scf_loge("\n");

	free(elf);
	elf = NULL;
	return -1;
}

int scf_elf_close(scf_elf_context_t* elf)
{
	if (elf) {
		if (elf->ops && elf->ops->close)
			elf->ops->close(elf);

		if (elf->fp)
			fclose(elf->fp);

		free(elf);
		elf = NULL;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

int scf_elf_add_sym(scf_elf_context_t* elf, const scf_elf_sym_t* sym, const char* sh_name)
{
	if (elf && sym && sh_name) {

		if (elf->ops && elf->ops->add_sym)
			return elf->ops->add_sym(elf, sym, sh_name);
	}

	scf_loge("\n");
	return -1;
}

int scf_elf_add_section(scf_elf_context_t* elf, const scf_elf_section_t* section)
{
	if (elf && section) {

		if (elf->ops && elf->ops->add_section)
			return elf->ops->add_section(elf, section);
	}

	scf_loge("\n");
	return -1;
}

int	scf_elf_add_rela_section(scf_elf_context_t* elf, const scf_elf_section_t* section, scf_vector_t* relas)
{
	if (elf && section && relas) {

		if (elf->ops && elf->ops->add_rela_section)
			return elf->ops->add_rela_section(elf, section, relas);
	}

	scf_loge("\n");
	return -1;
}

int	scf_elf_add_dyn_rela(scf_elf_context_t* elf, const scf_elf_rela_t* rela)
{
	if (elf && rela) {

		if (elf->ops && elf->ops->add_dyn_rela)
			return elf->ops->add_dyn_rela(elf, rela);
	}

	scf_loge("\n");
	return -1;
}

int	scf_elf_add_dyn_need(scf_elf_context_t* elf, const char* soname)
{
	if (elf && soname) {

		if (elf->ops && elf->ops->add_dyn_need)
			return elf->ops->add_dyn_need(elf, soname);
	}

	scf_loge("\n");
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

int scf_elf_read_syms(scf_elf_context_t* elf, scf_vector_t* syms, const char* sh_name)
{
	if (elf && syms && sh_name) {

		if (elf->ops && elf->ops->read_syms)
			return elf->ops->read_syms(elf, syms, sh_name);
	}

	scf_loge("\n");
	return -1;
}

int scf_elf_read_relas(scf_elf_context_t* elf, scf_vector_t* relas, const char* sh_name)
{
	if (elf && relas && sh_name) {

		if (elf->ops && elf->ops->read_relas)
			return elf->ops->read_relas(elf, relas, sh_name);
	}

	scf_loge("\n");
	return -1;
}

int scf_elf_write_rel(scf_elf_context_t* elf)
{
	if (elf && elf->ops && elf->ops->write_rel)
		return elf->ops->write_rel(elf);

	scf_loge("\n");
	return -1;
}

int scf_elf_write_exec(scf_elf_context_t* elf)
{
	if (elf && elf->ops && elf->ops->write_rel)
		return elf->ops->write_exec(elf);

	scf_loge("\n");
	return -1;
}

