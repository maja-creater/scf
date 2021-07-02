#include"scf_calculate.h"

#include"scf_calculate_i32.c"
#include"scf_calculate_u32.c"
#include"scf_calculate_i64.c"
#include"scf_calculate_float.c"
#include"scf_calculate_double.c"

scf_calculate_t  base_calculates[] =
{
	// i32
	{"i32", SCF_OP_ADD,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_add},
	{"i32", SCF_OP_SUB,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_sub},
	{"i32", SCF_OP_MUL,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_mul},
	{"i32", SCF_OP_DIV,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_div},
	{"i32", SCF_OP_MOD,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_mod},

	{"i32", SCF_OP_SHL,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_shl},
	{"i32", SCF_OP_SHR,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_shr},

	{"i32", SCF_OP_INC,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_inc},
	{"i32", SCF_OP_DEC,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_dec},

	{"i32", SCF_OP_NEG,       SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_neg},

	{"i32", SCF_OP_BIT_AND,   SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_bit_and},
	{"i32", SCF_OP_BIT_OR,    SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_bit_or},
	{"i32", SCF_OP_BIT_NOT,   SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_bit_not},

	{"i32", SCF_OP_LOGIC_AND, SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_logic_and},
	{"i32", SCF_OP_LOGIC_OR,  SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_logic_or},
	{"i32", SCF_OP_LOGIC_NOT, SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_logic_not},

	{"i32", SCF_OP_GT,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_gt},
	{"i32", SCF_OP_LT,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_lt},
	{"i32", SCF_OP_EQ,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_eq},
	{"i32", SCF_OP_NE,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_ne},
	{"i32", SCF_OP_GE,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_ge},
	{"i32", SCF_OP_LE,        SCF_VAR_I32, SCF_VAR_I32, SCF_VAR_I32, scf_i32_le},

	// u32
	{"u32", SCF_OP_ADD,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_add},
	{"u32", SCF_OP_SUB,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_sub},
	{"u32", SCF_OP_MUL,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_mul},
	{"u32", SCF_OP_DIV,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_div},
	{"u32", SCF_OP_MOD,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_mod},

	{"u32", SCF_OP_SHL,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_shl},
	{"u32", SCF_OP_SHR,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_shr},

	{"u32", SCF_OP_INC,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_inc},
	{"u32", SCF_OP_DEC,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_dec},

	{"u32", SCF_OP_NEG,       SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_neg},

	{"u32", SCF_OP_BIT_AND,   SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_bit_and},
	{"u32", SCF_OP_BIT_OR,    SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_bit_or},
	{"u32", SCF_OP_BIT_NOT,   SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_bit_not},

	{"u32", SCF_OP_LOGIC_AND, SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_logic_and},
	{"u32", SCF_OP_LOGIC_OR,  SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_logic_or},
	{"u32", SCF_OP_LOGIC_NOT, SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_logic_not},

	{"u32", SCF_OP_GT,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_gt},
	{"u32", SCF_OP_LT,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_lt},
	{"u32", SCF_OP_EQ,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_eq},
	{"u32", SCF_OP_NE,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_ne},
	{"u32", SCF_OP_GE,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_ge},
	{"u32", SCF_OP_LE,        SCF_VAR_U32, SCF_VAR_U32, SCF_VAR_U32, scf_u32_le},

	// i64
	{"i64", SCF_OP_ADD,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_add},
	{"i64", SCF_OP_SUB,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_sub},
	{"i64", SCF_OP_MUL,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_mul},
	{"i64", SCF_OP_DIV,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_div},
	{"i64", SCF_OP_MOD,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_mod},

	{"i64", SCF_OP_SHL,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_shl},
	{"i64", SCF_OP_SHR,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_shr},

	{"i64", SCF_OP_INC,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_inc},
	{"i64", SCF_OP_DEC,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_dec},

	{"i64", SCF_OP_NEG,       SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_neg},

	{"i64", SCF_OP_BIT_AND,   SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_bit_and},
	{"i64", SCF_OP_BIT_OR,    SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_bit_or},
	{"i64", SCF_OP_BIT_NOT,   SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_bit_not},

	{"i64", SCF_OP_LOGIC_AND, SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_logic_and},
	{"i64", SCF_OP_LOGIC_OR,  SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_logic_or},
	{"i64", SCF_OP_LOGIC_NOT, SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_logic_not},

	{"i64", SCF_OP_GT,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_gt},
	{"i64", SCF_OP_LT,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_lt},

	{"i64", SCF_OP_EQ,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_eq},
	{"i64", SCF_OP_NE,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_ne},
	{"i64", SCF_OP_GE,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_ge},
	{"i64", SCF_OP_LE,        SCF_VAR_I64, SCF_VAR_I64, SCF_VAR_I64, scf_i64_le},

	{"float", SCF_OP_ADD,       SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_add},
	{"float", SCF_OP_SUB,       SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_sub},
	{"float", SCF_OP_MUL,       SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_mul},
	{"float", SCF_OP_DIV,       SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_div},

	{"float", SCF_OP_NEG,       SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_neg},

	{"float", SCF_OP_GT,        SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_gt},
	{"float", SCF_OP_LT,        SCF_VAR_FLOAT, SCF_VAR_FLOAT, SCF_VAR_FLOAT, scf_float_lt},

	{"double", SCF_OP_ADD,       SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_add},
	{"double", SCF_OP_SUB,       SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_sub},
	{"double", SCF_OP_MUL,       SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_mul},
	{"double", SCF_OP_DIV,       SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_div},

	{"double", SCF_OP_NEG,       SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_neg},

	{"double", SCF_OP_GT,        SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_gt},
	{"double", SCF_OP_LT,        SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, SCF_VAR_DOUBLE, scf_double_lt},
};

scf_calculate_t* scf_find_base_calculate(int op_type, int src0_type, int src1_type)
{
	int i;
	for (i = 0; i < sizeof(base_calculates) / sizeof(base_calculates[0]); i++) {

		scf_calculate_t* cal = &base_calculates[i];

		if (op_type == cal->op_type
				&& src0_type == cal->src0_type
				&& src1_type == cal->src1_type) {
			return cal;
		}
	}

	return NULL;
}

