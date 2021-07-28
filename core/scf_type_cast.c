#include"scf_type_cast.h"

static int type_update[] =
{
	SCF_VAR_I8,
	SCF_VAR_CHAR,
	SCF_VAR_U8,

	SCF_VAR_I16,
	SCF_VAR_U16,

	SCF_VAR_INT,
	SCF_VAR_I32,
	SCF_VAR_U32,

	SCF_VAR_I64,
	SCF_VAR_INTPTR,

	SCF_VAR_U64,
	SCF_VAR_UINTPTR,

	SCF_VAR_FLOAT,
	SCF_VAR_DOUBLE,
};

static scf_type_cast_t  base_type_casts[] =
{
	{"char",    -1, SCF_VAR_CHAR,    scf_cast_to_u8},
	{"u8",      -1, SCF_VAR_U8,      scf_cast_to_u8},
	{"u16",     -1, SCF_VAR_U16,     scf_cast_to_u16},
	{"u32",     -1, SCF_VAR_U32,     scf_cast_to_u32},
	{"u64",     -1, SCF_VAR_U64,     scf_cast_to_u64},
	{"uintptr", -1, SCF_VAR_UINTPTR, scf_cast_to_u64},

	{"i8",      -1, SCF_VAR_I8,      scf_cast_to_i8},
	{"i16",     -1, SCF_VAR_I16,     scf_cast_to_i16},
	{"i32",     -1, SCF_VAR_I32,     scf_cast_to_i32},
	{"i64",     -1, SCF_VAR_I64,     scf_cast_to_i64},
//	{"intptr",  -1, SCF_VAR_INTPTR,  scf_cast_to_i64},

	{"float",   -1, SCF_VAR_FLOAT,   scf_cast_to_float},
//	{"double",  -1, SCF_VAR_DOUBLE,  scf_cast_to_double},

};

int scf_find_updated_type(scf_ast_t* ast, scf_variable_t* v0, scf_variable_t* v1)
{
	if (v0->nb_pointers > 0 || v1->nb_pointers > 0)
		return SCF_VAR_UINTPTR;

	int n = sizeof(type_update) / sizeof(type_update[0]);

	int index0 = -1;
	int index1 = -1;

	int i;
	for (i = 0; i < n; i++) {
		if (v0->type == type_update[i])
			index0 = i;

		if (v1->type == type_update[i])
			index1 = i;

		if (index0 >= 0 && index1 >= 0)
			break;
	}

	if (index0 < 0 || index1 < 0) {
		scf_loge("type update not found for type: %d, %d\n",
				v0->type, v1->type);
		return -1;
	}

	if (index0 > index1)
		return type_update[index0];

	return type_update[index1];
}

scf_type_cast_t* scf_find_base_type_cast(int src_type, int dst_type)
{
	int i;
	for (i = 0; i < sizeof(base_type_casts) / sizeof(base_type_casts[0]); i++) {

		scf_type_cast_t* cast = &base_type_casts[i];

		if (dst_type == cast->dst_type)
			return cast;
	}

	return NULL;
}

int scf_type_cast_check(scf_ast_t* ast, scf_variable_t* dst, scf_variable_t* src)
{
	if (!dst->const_flag && src->const_flag) {
		scf_logw("type cast %s -> %s discard 'const'\n",
				src->w->text->data, dst->w->text->data);
	}

	scf_string_t* dst_type = NULL;
	scf_string_t* src_type = NULL;

	int dst_nb_pointers = dst->nb_pointers + dst->nb_dimentions;
	int src_nb_pointers = src->nb_pointers + src->nb_dimentions;

	if (dst_nb_pointers > 0) {

		if (0 == src_nb_pointers) {
			if (SCF_VAR_INTPTR == src->type || SCF_VAR_UINTPTR == src->type
					|| SCF_VAR_U64 == src->type)
				return 0;
			goto failed;
		}

		if (SCF_VAR_VOID == src->type || SCF_VAR_VOID == dst->type)
			return 0;

		if (dst_nb_pointers != src_nb_pointers)
			goto failed;

		if (scf_type_is_integer(src->type) && scf_type_is_integer(dst->type)) {

			scf_type_t* t0 = NULL;
			scf_type_t* t1 = NULL;

			int ret = scf_ast_find_type_type(&t0, ast, src->type);
			if (ret < 0)
				goto failed;

			ret = scf_ast_find_type_type(&t1, ast, dst->type);
			if (ret < 0)
				goto failed;

			if (t0->size != t1->size)
				goto failed;

		} else if (src->type != dst->type)
			goto failed;

		return 0;
	}

	if (src_nb_pointers > 0) {
		if (SCF_VAR_INTPTR == dst->type || SCF_VAR_UINTPTR == dst->type)
			return 0;

		goto failed;
	}

	if (scf_type_is_integer(src->type)) {

		if (SCF_VAR_FLOAT <= dst->type && SCF_VAR_DOUBLE >= dst->type)
			return 0;

		if (scf_type_is_integer(dst->type)) {

			if (dst->size < src->size) {
				scf_logw("type cast %s -> %s discard bits\n",
						src->w->text->data, dst->w->text->data);
			}

			return 0;
		}
	}

	if (SCF_VAR_FLOAT <= src->type && SCF_VAR_DOUBLE >= src->type) {

		if (SCF_VAR_FLOAT <= dst->type && SCF_VAR_DOUBLE >= dst->type)
			return 0;

		if (scf_type_is_integer(dst->type))
			return 0;
	}

failed:
	dst_type = scf_variable_type_name(ast, dst);
	src_type = scf_variable_type_name(ast, src);

	scf_loge("type cast '%s -> %s' with different type: from '%s' to '%s'\n",
			src->w->text->data, dst->w->text->data,
			src_type->data, dst_type->data);

	scf_string_free(dst_type);
	scf_string_free(src_type);
	return -1;
}

