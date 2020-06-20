#include"scf_type_cast.h"

static scf_type_cast_t	type_casts[] =
{
	{SCF_VAR_FLOAT,   SCF_VAR_DOUBLE,  NULL},
	{SCF_VAR_FLOAT,   SCF_VAR_COMPLEX, NULL},

	{SCF_VAR_DOUBLE,  SCF_VAR_FLOAT,  NULL},

	{SCF_VAR_DOUBLE,  SCF_VAR_COMPLEX, NULL},

};


int scf_type_cast_check(scf_ast_t* ast, scf_variable_t* dst, scf_variable_t* src)
{
	if (!dst->const_flag && src->const_flag) {
		printf("%s(),%d, warning: type cast %s -> %s discard 'const'\n", __func__, __LINE__,
				src->w->text->data, dst->w->text->data);
	}

	if (dst->nb_pointers > 0 && src->nb_pointers > 0) {
		if (dst->type != src->type || dst->nb_pointers != src->nb_pointers) {
			printf("%s(),%d, warning: type cast %s -> %s with different type pointer\n",
					__func__, __LINE__, src->w->text->data, dst->w->text->data);
		}
		return 0;
	}

	if (dst->nb_pointers > 0 && (SCF_VAR_INTPTR == src->type || SCF_VAR_UINTPTR == src->type))
		return 0;
	if (src->nb_pointers > 0 && (SCF_VAR_INTPTR == dst->type || SCF_VAR_UINTPTR == dst->type))
		return 0;

	if (scf_type_is_integer(src->type)) {

		if (SCF_VAR_FLOAT <= dst->type && SCF_VAR_COMPLEX >= dst->type)
			return 0;

		if (scf_type_is_integer(dst->type)) {

			if (dst->size <= src->size) {
				printf("%s(),%d, warning: type cast %s -> %s discard sign bit or other bits\n",
						__func__, __LINE__,
						src->w->text->data, dst->w->text->data);
			}

			return 0;
		}
	}

	int i;
	for (i = 0; i < sizeof(type_casts) / sizeof(type_casts[0]); i++) {

		scf_type_cast_t* cast = &type_casts[i];

		if (dst->type == cast->dst_type && src->type ==  cast->src_type)
			return 0;
	}

	return -1;
}

