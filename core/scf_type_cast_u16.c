#include"scf_type_cast.h"

int scf_cast_to_u16(scf_ast_t* ast, scf_variable_t** pret, scf_variable_t* src)
{
	if (!pret || !src)
		return -EINVAL;

	scf_type_t*     t = scf_ast_find_type_type(ast, SCF_VAR_U16);

	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(src->w, t, src->const_flag, 0, NULL);

	switch (src->type) {
		case SCF_VAR_FLOAT:
			r->data.u32 = (uint32_t)src->data.f;
			break;

		case SCF_VAR_DOUBLE:
			r->data.u32 = (uint32_t)src->data.d;
			break;

		case SCF_VAR_CHAR:
		case SCF_VAR_I8:
		case SCF_VAR_U8:
		case SCF_VAR_I16:
		case SCF_VAR_U16:
		case SCF_VAR_I32:
		case SCF_VAR_U32:
		case SCF_VAR_I64:
		case SCF_VAR_U64:
		case SCF_VAR_INTPTR:
		case SCF_VAR_UINTPTR:
			r->data.u32 = src->data.u32;
			break;

		default:
			if (src->nb_pointers > 0)
				r->data.u32 = src->data.u32;
			else {
				scf_variable_free(r);
				return -EINVAL;
			}
			break;
	};

	*pret = r;
	return 0;
}
