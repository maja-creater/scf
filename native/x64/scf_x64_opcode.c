#include"scf_x64.h"

scf_x64_OpCode_t	x64_OpCodes[] = {
	{SCF_X64_PUSH, "push", 1, {0x50, 0x0, 0x0},1,  8,8, SCF_X64_G,   0,0, 0,{0,0}},
	{SCF_X64_POP,  "pop",  1, {0x58, 0x0, 0x0},1,  8,8, SCF_X64_G,   0,0, 0,{0,0}},

	{SCF_X64_INC,  "inc",  2, {0xfe, 0x0, 0x0},1,  1,1, SCF_X64_E,   0,1, 0,{0,0}},
	{SCF_X64_INC,  "inc",  2, {0xff, 0x0, 0x0},1,  2,2, SCF_X64_E,   0,1, 0,{0,0}},
	{SCF_X64_INC,  "inc",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_E,   0,1, 0,{0,0}},
	{SCF_X64_INC,  "inc",  2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E,   0,1, 0,{0,0}},

	{SCF_X64_DEC,  "dec",  2, {0xfe, 0x0, 0x0},1,  1,1, SCF_X64_E,   1,1, 0,{0,0}},
	{SCF_X64_DEC,  "dec",  2, {0xff, 0x0, 0x0},1,  2,2, SCF_X64_E,   1,1, 0,{0,0}},
	{SCF_X64_DEC,  "dec",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_E,   1,1, 0,{0,0}},
	{SCF_X64_DEC,  "dec",  2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E,   1,1, 0,{0,0}},

	{SCF_X64_XOR,  "xor",  2, {0x30, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x32, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},
#if 0
	{SCF_X64_XOR,  "xor",  2, {0x34, 0x0, 0x0},1,  1,1, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  2,2, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  4,4, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  4,8, SCF_X64_I2G, 0,0, 1,{0,0}},
#endif
	{SCF_X64_AND,  "and",  2, {0x20, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x21, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x21, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x21, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x22, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x23, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x23, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x23, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x81, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_AND,  "and",  2, {0x81, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 4,1, 0,{0,0}},

	{SCF_X64_OR,   "or",   2, {0x08, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x09, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x09, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x09, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x0a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x0b, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x0b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x0b, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 1,1, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x81, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 1,1, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 1,1, 0,{0,0}},
	{SCF_X64_OR,   "or",   2, {0x81, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 1,1, 0,{0,0}},

	{SCF_X64_CALL, "call", 5, {0xe8, 0x0, 0x0},1,  4,4, SCF_X64_I,   0,0, 0,{0,0}},
	{SCF_X64_CALL, "call", 2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E,   2,1, 0,{0,0}},

	{SCF_X64_RET,  "ret",  1, {0xc3, 0x0, 0x0},1,  8,8, SCF_X64_G,   0,0, 0,{0,0}},

	{SCF_X64_ADD,  "add",  2, {0x00, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x01, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x01, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x01, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x02, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x03, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x03, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x03, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	// add r/m, imm
	{SCF_X64_ADD,  "add",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x81, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x81, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 0,1, 0,{0,0}},

	// add r/m, imm8
	{SCF_X64_ADD,  "add",  2, {0x83, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_ADD,  "add",  2, {0x83, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 0,1, 0,{0,0}},


	{SCF_X64_SUB,  "sub",  2, {0x28, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x29, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x29, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x29, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},

	{SCF_X64_SUB,  "sub",  2, {0x2a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x2b, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x2b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x2b, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_SUB,  "sub",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x81, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x81, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x83, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x83, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 5,1, 0,{0,0}},

	{SCF_X64_MUL,  "mul",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   4,1, 2,{0,2}},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   4,1, 2,{0,2}},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   4,1, 2,{0,2}},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   4,1, 2,{0,2}},

	{SCF_X64_IMUL, "imul", 2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   5,1, 2,{0,2}},
	{SCF_X64_IMUL, "imul", 2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   5,1, 2,{0,2}},
	{SCF_X64_IMUL, "imul", 2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   5,1, 2,{0,2}},
	{SCF_X64_IMUL, "imul", 2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   5,1, 2,{0,2}},

	{SCF_X64_DIV,  "div",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   6,1, 2,{0,2}},
	{SCF_X64_DIV,  "div",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   6,1, 2,{0,2}},
	{SCF_X64_DIV,  "div",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   6,1, 2,{0,2}},
	{SCF_X64_DIV,  "div",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   6,1, 2,{0,2}},

	{SCF_X64_IDIV, "idiv", 2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   7,1, 2,{0,2}},
	{SCF_X64_IDIV, "idiv", 2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   7,1, 2,{0,2}},
	{SCF_X64_IDIV, "idiv", 2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   7,1, 2,{0,2}},
	{SCF_X64_IDIV, "idiv", 2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   7,1, 2,{0,2}},

	{SCF_X64_CBW,  "cwd",  1, {0x98, 0x0, 0x0},1,  1,2, SCF_X64_G,   0,0, 2,{0,2}},
	{SCF_X64_CWD,  "cwd",  1, {0x98, 0x0, 0x0},1,  2,4, SCF_X64_G,   0,0, 2,{0,2}},
	{SCF_X64_CDQ,  "cdq",  1, {0x98, 0x0, 0x0},1,  4,8, SCF_X64_G,   0,0, 2,{0,2}},
	{SCF_X64_CQO,  "cqo",  1, {0x98, 0x0, 0x0},1,  8,16, SCF_X64_G,  0,0, 2,{0,2}},

	{SCF_X64_SAR,  "sar",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xd2, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 7,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SAR,  "sar",  2, {0xd3, 0x0, 0x0},1,  1,2, SCF_X64_G2E, 7,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SAR,  "sar",  2, {0xd3, 0x0, 0x0},1,  1,4, SCF_X64_G2E, 7,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SAR,  "sar",  2, {0xd3, 0x0, 0x0},1,  1,8, SCF_X64_G2E, 7,1, 1,{SCF_X64_REG_CL,0}},

	{SCF_X64_SHR,  "shr",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xd2, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 5,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHR,  "shr",  2, {0xd3, 0x0, 0x0},1,  1,2, SCF_X64_G2E, 5,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHR,  "shr",  2, {0xd3, 0x0, 0x0},1,  1,4, SCF_X64_G2E, 5,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHR,  "shr",  2, {0xd3, 0x0, 0x0},1,  1,8, SCF_X64_G2E, 5,1, 1,{SCF_X64_REG_CL,0}},

	{SCF_X64_SHL,  "shl",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xd2, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 4,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHL,  "shl",  2, {0xd3, 0x0, 0x0},1,  1,2, SCF_X64_G2E, 4,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHL,  "shl",  2, {0xd3, 0x0, 0x0},1,  1,4, SCF_X64_G2E, 4,1, 1,{SCF_X64_REG_CL,0}},
	{SCF_X64_SHL,  "shl",  2, {0xd3, 0x0, 0x0},1,  1,8, SCF_X64_G2E, 4,1, 1,{SCF_X64_REG_CL,0}},

	{SCF_X64_NEG,  "neg",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   3,1, 0,{0,0}},

	{SCF_X64_NOT,  "not",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   2,1, 0,{0,0}},
	{SCF_X64_NOT,  "not",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   2,1, 0,{0,0}},
	{SCF_X64_NOT,  "not",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   2,1, 0,{0,0}},
	{SCF_X64_NOT,  "not",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   2,1, 0,{0,0}},

	{SCF_X64_LEA,  "lea",  1, {0x8d, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MOV,  "mov",  2, {0x88, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x89, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x89, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x89, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x8a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x8b, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x8b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0x8b, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MOV,  "mov",  2, {0xb0, 0x0, 0x0},1,  1,1, SCF_X64_I2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xb8, 0x0, 0x0},1,  2,2, SCF_X64_I2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xb8, 0x0, 0x0},1,  4,4, SCF_X64_I2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xb8, 0x0, 0x0},1,  8,8, SCF_X64_I2G, 0,0, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xc6, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xc7, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xc7, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_MOV,  "mov",  2, {0xc7, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 0,1, 0,{0,0}},

	{SCF_X64_MOVSX, "movsx",  2, {0x0f, 0xbe, 0x0},2, 1,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSX, "movsx",  2, {0x0f, 0xbe, 0x0},2, 1,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSX, "movsx",  2, {0x0f, 0xbe, 0x0},2, 1,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSX, "movsx",  2, {0x0f, 0xbf, 0x0},2, 2,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSX, "movsx",  2, {0x0f, 0xbf, 0x0},2, 2,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSX, "movsx",  2, {0x63, 0x0, 0x0}, 1, 4,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MOVZX, "movzx",  2, {0x0f, 0xb6, 0x0},2, 1,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVZX, "movzx",  2, {0x0f, 0xb6, 0x0},2, 1,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVZX, "movzx",  2, {0x0f, 0xb6, 0x0},2, 1,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVZX, "movzx",  2, {0x0f, 0xb7, 0x0},2, 2,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVZX, "movzx",  2, {0x0f, 0xb7, 0x0},2, 2,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MOVS, "movs",  2, {0xf3, 0xa4, 0x0},2, 1,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_MOVS, "movs",  2, {0xf3, 0xa4, 0x0},2, 1,8, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_MOVS, "movs",  2, {0xf3, 0xa5, 0x0},2, 2,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_MOVS, "movs",  2, {0xf3, 0xa5, 0x0},2, 4,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_MOVS, "movs",  2, {0xf3, 0xa5, 0x0},2, 8,8, SCF_X64_G, 0,0, 0,{0,0}},

	{SCF_X64_STOS, "stos",  2, {0xf3, 0xaa, 0x0},2, 1,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_STOS, "stos",  2, {0xf3, 0xaa, 0x0},2, 1,8, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_STOS, "stos",  2, {0xf3, 0xab, 0x0},2, 2,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_STOS, "stos",  2, {0xf3, 0xab, 0x0},2, 4,4, SCF_X64_G, 0,0, 0,{0,0}},
	{SCF_X64_STOS, "stos",  2, {0xf3, 0xab, 0x0},2, 8,8, SCF_X64_G, 0,0, 0,{0,0}},

	{SCF_X64_CMP,  "cmp",  2, {0x38, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x39, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x39, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x39, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x3a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x3b, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x3b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x3b, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CMP,  "cmp",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x81, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x81, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x83, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_CMP,  "cmp",  2, {0x83, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 7,1, 0,{0,0}},

	{SCF_X64_TEST, "test", 2, {0x84, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0x85, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0x85, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0x85, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},

	{SCF_X64_TEST, "test", 2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0,1, 0,{0,0}},
	{SCF_X64_TEST, "test", 2, {0xf7, 0x0, 0x0},1,  4,8, SCF_X64_I2E, 0,1, 0,{0,0}},

	{SCF_X64_SETZ,  "setz",  3, {0x0f, 0x94, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETNZ, "setnz", 3, {0x0f, 0x95, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_SETG,  "setg",  3, {0x0f, 0x9f, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETGE, "setge", 3, {0x0f, 0x9d, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_SETL,  "setl",  3, {0x0f, 0x9c, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETLE, "setle", 3, {0x0f, 0x9e, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_ADDSS, "addss", 4, {0xf3, 0x0f, 0x58},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_ADDSD, "addsd", 8, {0xf2, 0x0f, 0x58},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_SUBSS, "subss", 4, {0xf3, 0x0f, 0x5c},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_SUBSD, "subsd", 8, {0xf2, 0x0f, 0x5c},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MULSS, "mulss", 4, {0xf3, 0x0f, 0x59},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MULSD, "mulsd", 8, {0xf2, 0x0f, 0x59},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_DIVSS, "divss", 4, {0xf3, 0x0f, 0x5e},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_DIVSD, "divsd", 8, {0xf2, 0x0f, 0x5e},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_MOVSS, "movss", 4, {0xf3, 0x0f, 0x10},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSS, "movss", 4, {0xf3, 0x0f, 0x11},3, 4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_MOVSD, "movsd", 8, {0xf2, 0x0f, 0x10},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_MOVSD, "movsd", 8, {0xf2, 0x0f, 0x11},3, 8,8, SCF_X64_G2E, 0,0, 0,{0,0}},

	{SCF_X64_PXOR,  "pxor",  8, {0x0f, 0xef, 0   },2, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_PXOR,  "pxor",  8, {0x66, 0x0f, 0xef},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_UCOMISS, "ucomiss", 3, {0x0f, 0x2e, 0},   2, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_UCOMISD, "ucomisd", 4, {0x66, 0x0f, 0x2e},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CVTSS2SD, "cvtss2sd", 4, {0xf3, 0x0f, 0x5a},3, 4,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CVTSD2SS, "cvtsd2ss", 4, {0xf2, 0x0f, 0x5a},3, 8,4, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CVTSI2SS, "cvtsi2ss", 4, {0xf3, 0x0f, 0x2a},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CVTSI2SD, "cvtsi2sd", 4, {0xf2, 0x0f, 0x2a},3, 4,8, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CVTSI2SD, "cvtsi2sd", 4, {0xf2, 0x0f, 0x2a},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CVTTSS2SI, "cvttss2si", 4, {0xf3, 0x0f, 0x2c},3, 4,4, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_CVTTSD2SI, "cvttsd2si", 8, {0xf2, 0x0f, 0x2c},3, 8,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_CVTTSD2SI, "cvttsd2si", 8, {0xf2, 0x0f, 0x2c},3, 8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_JZ,   "jz",   2, {0x74, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JZ,   "jz",   6, {0x0f, 0x84, 0x0},2, 4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JNZ,  "jnz",  2, {0x75, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JNZ,  "jnz",  6, {0x0f, 0x85, 0x0},2, 4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JG,   "jg",   2, {0x7f, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JG,   "jg",   6, {0x0f, 0x8f,0x0},1,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JGE,  "jge",  2, {0x7d, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JGE,  "jge",  6, {0x0f, 0x8d,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JL,   "jl",   2, {0x7c, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JL,   "jl",   6, {0x0f, 0x8c,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JLE,  "jle",  2, {0x7e, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JLE,  "jle",  6, {0x0f, 0x8e,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JA,   "ja",   2, {0x77, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JA,   "ja",   6, {0x0f, 0x87,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JAE,  "jae",  2, {0x73, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JAE,  "jae",  6, {0x0f, 0x83,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JB,   "jb",   2, {0x72, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JB,   "jb",   6, {0x0f, 0x82,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JBE,  "jbe",  2, {0x76, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JBE,  "jbe",  6, {0x0f, 0x86,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JMP,  "jmp",  2, {0xeb, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JMP,  "jmp",  5, {0xe9, 0x0, 0x0},1,  4,4, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JMP,  "jmp",  2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E, 4,1, 0,{0,0}},
};

scf_x64_OpCode_t*   x64_find_OpCode_by_type(const int type)
{
	int i;
	for (i = 0; i < sizeof(x64_OpCodes) / sizeof(x64_OpCodes[0]); i++) {

		scf_x64_OpCode_t* OpCode = &(x64_OpCodes[i]);
		if (OpCode->type == type)
			return OpCode;
	}
	return NULL;
}

scf_x64_OpCode_t* x64_find_OpCode(const int type, const int OpBytes, const int RegBytes, const int EG)
{
	int i;
	for (i = 0; i < sizeof(x64_OpCodes) / sizeof(x64_OpCodes[0]); i++) {

		scf_x64_OpCode_t* OpCode = &(x64_OpCodes[i]);

		if (type == OpCode->type
				&& OpBytes == OpCode->OpBytes
				&& RegBytes == OpCode->RegBytes
				&& EG == OpCode->EG)
			return OpCode;
	}
	return NULL;
}

int x64_find_OpCodes(scf_vector_t* results, const int type, const int OpBytes, const int RegBytes, const int EG)
{
	int i;
	for (i = 0; i < sizeof(x64_OpCodes) / sizeof(x64_OpCodes[0]); i++) {

		scf_x64_OpCode_t* OpCode = &(x64_OpCodes[i]);

		if (type == OpCode->type
				&& OpBytes == OpCode->OpBytes
				&& RegBytes == OpCode->RegBytes
				&& EG == OpCode->EG) {

			int ret = scf_vector_add(results, OpCode);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

