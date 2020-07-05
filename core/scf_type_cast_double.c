#include"scf_type_cast.h"

int scf_cast_to_double(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src)
{
	if (!pret || !src)
		return -EINVAL;

	scf_type_t*     t = scf_ast_find_type_type(ast, SCF_VAR_DOUBLE);

	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(src->w, t, 0, 0, NULL);

	switch (src->type) {
		case SCF_VAR_FLOAT:
			r->data.d = src->data.f;
			break;

		case SCF_VAR_CHAR:
		case SCF_VAR_I8:
		case SCF_VAR_U8:
		case SCF_VAR_I16:
		case SCF_VAR_U16:
		case SCF_VAR_I32:
			r->data.d = (double)src->data.i;
			break;
		case SCF_VAR_U32:
			r->data.d = (double)src->data.u32;
			break;

		case SCF_VAR_I64:
		case SCF_VAR_INTPTR:
			r->data.d = (double)src->data.i64;
			break;

		case SCF_VAR_U64:
		case SCF_VAR_UINTPTR:
			r->data.d = (double)src->data.u64;
			break;

		default:
			if (src->nb_pointers > 0)
				r->data.d = (double)src->data.u64;
			else {
				scf_variable_free(r);
				return -EINVAL;
			}
			break;
	};

	*pret = r;
	return 0;
}