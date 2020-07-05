#include"scf_x64.h"
#include"scf_elf.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

extern scf_native_ops_t native_ops_x64;

scf_register_x64_t	x64_registers[] = {
	{0, 1, "al",    X64_COLOR(0, 0x1), NULL},
	{0, 2, "ax",    X64_COLOR(0, 0x3), NULL},
	{0, 4, "eax",   X64_COLOR(0, 0xf), NULL},
	{0, 8, "rax",   X64_COLOR(0, 0xff), NULL},

	{1, 1, "cl",    X64_COLOR(1, 0x1), NULL},
	{1, 2, "cx",    X64_COLOR(1, 0x3), NULL},
	{1, 4, "ecx",   X64_COLOR(1, 0xf), NULL},
	{1, 8, "rcx",   X64_COLOR(1, 0xff), NULL},

	{2, 1, "dl",    X64_COLOR(2, 0x1), NULL},
	{2, 2, "dx",    X64_COLOR(2, 0x3), NULL},
	{2, 4, "edx",   X64_COLOR(2, 0xf), NULL},
	{2, 8, "rdx",   X64_COLOR(2, 0xff), NULL},

	{3, 1, "bl",    X64_COLOR(3, 0x1), NULL},
	{3, 2, "bx",    X64_COLOR(3, 0x3), NULL},
	{3, 4, "ebx",   X64_COLOR(3, 0xf), NULL},
	{3, 8, "rbx",   X64_COLOR(3, 0xff), NULL},
#if 1
	{4, 2, "sp",    X64_COLOR(4, 0x3), NULL},
	{4, 4, "esp",   X64_COLOR(4, 0xf), NULL},
	{4, 8, "rsp",   X64_COLOR(4, 0xff), NULL},

	{5, 2, "bp",    X64_COLOR(5, 0x3), NULL},
	{5, 4, "ebp",   X64_COLOR(5, 0xf), NULL},
	{5, 8, "rbp",   X64_COLOR(5, 0xff), NULL},

	{6, 2, "si",    X64_COLOR(6, 0x3), NULL},
	{6, 4, "esi",   X64_COLOR(6, 0xf), NULL},
	{6, 8, "rsi",   X64_COLOR(6, 0xff), NULL},

	{7, 2, "di",    X64_COLOR(7, 0x3), NULL},
	{7, 4, "edi",   X64_COLOR(7, 0xf), NULL},
	{7, 8, "rdi",   X64_COLOR(7, 0xff), NULL},
#endif
	{4, 1, "ah",    X64_COLOR(0, 0x2), NULL},
	{5, 1, "ch",    X64_COLOR(1, 0x2), NULL},
	{6, 1, "dh",    X64_COLOR(2, 0x2), NULL},
	{7, 1, "bh",    X64_COLOR(3, 0x2), NULL},
};

scf_x64_OpCode_t	x64_OpCodes[] = {
	{SCF_X64_PUSH, "push", 1, {0x50, 0x0, 0x0},1,  8,8, SCF_X64_G,   0,0, 0,{0,0}},
	{SCF_X64_POP,  "pop",  1, {0x58, 0x0, 0x0},1,  8,8, SCF_X64_G,   0,0, 0,{0,0}},

	{SCF_X64_INC,  "inc",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_G,   0,1, 0,{0,0}},
	{SCF_X64_DEC,  "dec",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_G,   1,1, 0,{0,0}},

	{SCF_X64_XOR,  "xor",  2, {0x30, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  2,2, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  8,8, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x32, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  2,2, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  8,8, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_XOR,  "xor",  2, {0x34, 0x0, 0x0},1,  1,1, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  2,2, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  4,4, SCF_X64_I2G, 0,0, 1,{0,0}},
	{SCF_X64_XOR,  "xor",  2, {0x35, 0x0, 0x0},1,  4,8, SCF_X64_I2G, 0,0, 1,{0,0}},

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

	// add ax, imm
	{SCF_X64_ADD,  "add",  2, {0x04, 0x0, 0x0},1,  1,1, SCF_X64_I2G, 0,0, 1,{SCF_X64_REG_AL,0}},
	{SCF_X64_ADD,  "add",  2, {0x05, 0x0, 0x0},1,  2,2, SCF_X64_I2G, 0,0, 1,{SCF_X64_REG_AX,0}},
	{SCF_X64_ADD,  "add",  2, {0x05, 0x0, 0x0},1,  4,4, SCF_X64_I2G, 0,0, 1,{SCF_X64_REG_EAX,0}},
	{SCF_X64_ADD,  "add",  2, {0x05, 0x0, 0x0},1,  4,8, SCF_X64_I2G, 0,0, 1,{SCF_X64_REG_RAX,0}},

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
	{SCF_X64_SUB,  "sub",  2, {0x29, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x2a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x2b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0, 0,{0,0}},

	{SCF_X64_SUB,  "sub",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SUB,  "sub",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 5,1, 0,{0,0}},

	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   4,1, 2,{0,2}},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   4,1, 2,{0,2}},

	{SCF_X64_IMUL, "imul", 2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   5,1, 2,{0,2}},
	{SCF_X64_IMUL, "imul", 2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   5,1, 2,{0,2}},

	{SCF_X64_DIV,  "div",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   6,1, 2,{0,2}},
	{SCF_X64_DIV,  "div",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   6,1, 2,{0,2}},

	{SCF_X64_IDIV, "idiv", 2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   7,1, 2,{0,2}},
	{SCF_X64_IDIV, "idiv", 2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   7,1, 2,{0,2}},

	{SCF_X64_CWD,  "cwd",  1, {0x99, 0x0, 0x0},1,  2,4, SCF_X64_G,   0,0, 2,{0,2}},
	{SCF_X64_CDQ,  "cdq",  1, {0x99, 0x0, 0x0},1,  4,8, SCF_X64_G,   0,0, 2,{0,2}},
	{SCF_X64_CQO,  "cqo",  1, {0x99, 0x0, 0x0},1,  8,16, SCF_X64_G,  0,0, 2,{0,2}},

	{SCF_X64_SAR,  "sar",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 7,1, 0,{0,0}},
	{SCF_X64_SAR,  "sar",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 7,1, 0,{0,0}},

	{SCF_X64_SHR,  "shr",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 5,1, 0,{0,0}},
	{SCF_X64_SHR,  "shr",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 5,1, 0,{0,0}},

	{SCF_X64_SHL,  "shl",  2, {0xc0, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,2, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 4,1, 0,{0,0}},
	{SCF_X64_SHL,  "shl",  2, {0xc1, 0x0, 0x0},1,  1,8, SCF_X64_I2E, 4,1, 0,{0,0}},

	{SCF_X64_NEG,  "neg",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E,   3,1, 0,{0,0}},
	{SCF_X64_NEG,  "neg",  2, {0xf7, 0x0, 0x0},1,  8,8, SCF_X64_E,   3,1, 0,{0,0}},

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

	{SCF_X64_SETZ,  "setz",  3, {0x0f, 0x94, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETNZ, "setnz", 3, {0x0f, 0x95, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_SETG,  "setg",  3, {0x0f, 0x9f, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETGE, "setge", 3, {0x0f, 0x9d, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_SETL,  "setl",  3, {0x0f, 0x9c, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},
	{SCF_X64_SETLE, "setle", 3, {0x0f, 0x9e, 0x0},2,  1,1, SCF_X64_E, 0,0, 0,{0,0}},

	{SCF_X64_JZ,   "jz",   2, {0x74, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JZ,   "jz",   6, {0x0f, 0x84, 0x0},2, 4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JNZ,  "jnz",  2, {0x75, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JNZ,  "jnz",  6, {0x0f, 0x85, 0x0},2, 4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JG,   "jg",   2, {0x7f, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JG,   "jg",   6, {0x0f, 0x8f,0x0},1,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JGE,  "jge",  2, {0x7d, 0x0, 0x0},2,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JGE,  "jge",  6, {0x0f, 0x8d,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JL,   "jl",   2, {0x7c, 0x0, 0x0},2,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JL,   "jl",   6, {0x0f, 0x8c,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JLE,  "jle",  2, {0x7e, 0x0, 0x0},2,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JLE,  "jle",  6, {0x0f, 0x8e,0x0},2,  4,4, SCF_X64_I, 0,0, 0,{0,0}},

	{SCF_X64_JMP,  "jmp",  2, {0xeb, 0x0, 0x0},1,  1,1, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JMP,  "jmp",  5, {0xe9, 0x0, 0x0},1,  4,4, SCF_X64_I, 0,0, 0,{0,0}},
	{SCF_X64_JMP,  "jmp",  2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E, 4,1, 0,{0,0}},
};

static int _x64_registers_init()
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		assert(!r->dag_nodes);

		r->dag_nodes = scf_vector_alloc();
		if (!r->dag_nodes)
			return -ENOMEM;
	}

	return 0;
}

static void _x64_registers_clear()
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (r->dag_nodes) {
			scf_vector_free(r->dag_nodes);
			r->dag_nodes = NULL;
		}
	}
}

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

scf_register_x64_t* x64_find_register(const char* name)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (!strcmp(r->name, name))
			return r;
	}
	return NULL;
}

scf_register_x64_t* x64_find_register_id_bytes(uint32_t id, int bytes)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (r->id == id && r->bytes == bytes)
			return r;
	}
	return NULL;
}

scf_register_x64_t* x64_find_register_color(intptr_t color)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (r->color == color)
			return r;
	}
	return NULL;
}

int	scf_x64_open(scf_native_t* ctx)
{
	scf_x64_context_t* x64 = calloc(1, sizeof(scf_x64_context_t));
	if (!x64)
		return -ENOMEM;

	ctx->priv = x64;
	return 0;
}

int scf_x64_close(scf_native_t* ctx)
{
	scf_x64_context_t* x64 = ctx->priv;

	if (x64) {
		_x64_registers_clear();

		free(x64);
		x64 = NULL;
	}
	return 0;
}

static int _x64_function_init(scf_function_t* f, scf_vector_t* local_vars)
{
	int ret = _x64_registers_init();
	if (ret < 0)
		return ret;

	int i;
	int local_vars_size = 0;

	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];

		// align 8 bytes
		if (local_vars_size & 0x7) {
			local_vars_size = (local_vars_size + 7) >> 3 << 3;
		}

		// local var in the low memory address based on rbp
		// (rbp) is old rbp, -8(rbp) is 1st local var
		v->bp_offset	= -8 - local_vars_size;
		v->local_flag	= 1;
		local_vars_size	+= v->size;
	}

	if (!f->init_insts) {
		f->init_insts = scf_vector_alloc();
		if (!f->init_insts)
			return -ENOMEM;
	} else
		scf_vector_clear(f->init_insts, free);

	scf_x64_OpCode_t* push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* pop  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);
	scf_x64_OpCode_t* mov  = x64_find_OpCode(SCF_X64_MOV,  4,4, SCF_X64_G2E);
	scf_x64_OpCode_t* sub  = x64_find_OpCode(SCF_X64_SUB,  4,4, SCF_X64_I2E);

	scf_register_x64_t* rsp = x64_find_register("rsp");
	scf_register_x64_t* rbp = x64_find_register("rbp");

	ret = scf_vector_add(f->init_insts, x64_make_inst_G(push, rbp));
	if (ret < 0)
		return ret;

	ret = scf_vector_add(f->init_insts, x64_make_inst_G2E(mov, rbp, rsp));
	if (ret < 0)
		return ret;

	ret = scf_vector_add(f->init_insts, x64_make_inst_I2E(sub, rsp, (uint8_t*)&local_vars_size, sizeof(int)));
	if (ret < 0)
		return ret;

	return local_vars_size;
}

static int _x64_function_finish(scf_function_t* f)
{
	if (!f->fini_insts) {
		f->fini_insts= scf_vector_alloc();
		if (!f->fini_insts)
			return -ENOMEM;
	} else
		scf_vector_clear(f->fini_insts, free);

	scf_x64_OpCode_t* push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* pop  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);
	scf_x64_OpCode_t* mov  = x64_find_OpCode(SCF_X64_MOV,  4,4, SCF_X64_G2E);

	scf_register_x64_t* rsp = x64_find_register("rsp");
	scf_register_x64_t* rbp = x64_find_register("rbp");

	int ret = scf_vector_add(f->fini_insts, x64_make_inst_G2E(mov, rsp, rbp));
	if (ret < 0)
		return ret;

	ret = scf_vector_add(f->fini_insts, x64_make_inst_G(pop, rbp));
	if (ret < 0)
		return ret;

	_x64_registers_clear();
	return 0;
}

static void _x64_rcg_node_printf(x64_rcg_node_t* rn)
{
	if (rn->dag_node) {
		if (rn->dag_node->var->w) {
			scf_logw("v_%d_%d/%s, color: %ld, major: %ld, minor: %ld\n",
					rn->dag_node->var->w->line, rn->dag_node->var->w->pos,
					rn->dag_node->var->w->text->data,
					rn->dag_node->color,
					X64_COLOR_MAJOR(rn->dag_node->color),
					X64_COLOR_MINOR(rn->dag_node->color));
		} else {
			scf_logw("v_%#lx, color: %ld, major: %ld, minor: %ld\n",
					(uintptr_t)rn->dag_node->var & 0xffff, rn->dag_node->color,
					X64_COLOR_MAJOR(rn->dag_node->color),
					X64_COLOR_MINOR(rn->dag_node->color));
		}
	} else if (rn->reg) {
		scf_logw("r/%s, color: %ld, major: %ld, minor: %ld\n",
				rn->reg->name, rn->reg->color,
				X64_COLOR_MAJOR(rn->reg->color),
				X64_COLOR_MINOR(rn->reg->color));
	}
}

static void _x64_inst_printf(scf_3ac_code_t* c)
{
	if (!c->instructions)
		return;

	int i;
	for (i = 0; i < c->instructions->size; i++) {
		scf_instruction_t* inst = c->instructions->data[i];
		int j;
		for (j = 0; j < inst->len; j++)
			printf("%02x ", inst->code[j]);
		printf("\n");
	}
	printf("\n");
}

int x64_overflow_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f)
{
	int i;
	int j;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		j = 0;
		while (j < r2->dag_nodes->size) {
			scf_dag_node_t* dn = r2->dag_nodes->data[j];

			assert(dn->var->size == r2->bytes);

			if (dn->var->global_flag || dn->var->local_flag) {
				scf_logw("save var: v_%#lx\n", 0xffff & (uintptr_t)dn->var);

				scf_x64_OpCode_t*  mov;
				scf_instruction_t* inst;
				scf_rela_t*        rela = NULL;

				mov  = x64_find_OpCode(SCF_X64_MOV, r2->bytes, r2->bytes, SCF_X64_G2E);
				inst = x64_make_inst_G2M(&rela, mov, dn->var, NULL, r2);
				X64_INST_ADD_CHECK(c->instructions, inst);
				X64_RELA_ADD_CHECK(f->data_relas, rela, c, dn->var, NULL);

				// if this var is function argment, it become a normal local var
				dn->var->arg_flag = 0;

			} else {
				scf_logw("r2: %s, color: %ld:%ld, temp var not save, ", r2->name,
						X64_COLOR_MAJOR(r2->color),
						X64_COLOR_MINOR(r2->color));
				if (dn->var->w)
					printf("v_%d_%d/%s\n", dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
				else
					printf("v_%#lx\n", 0xffff & (uintptr_t)dn->var);
			}

			dn->color = -1;

			int ret = scf_vector_del(r2->dag_nodes, dn);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}
	}

	return 0;
}

int x64_reg_cached_vars(scf_register_x64_t* r)
{
	int nb_vars = 0;
	int i;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		nb_vars += r2->dag_nodes->size;
	}

	return nb_vars;
}

scf_register_x64_t* x64_select_overflowed_reg(scf_3ac_code_t* c, int bytes)
{
	scf_register_x64_t* active_regs[sizeof(x64_registers) / sizeof(x64_registers[0])];
	scf_register_x64_t* free_regs[  sizeof(x64_registers) / sizeof(x64_registers[0])];

	int nb_active_regs = 0;
	int nb_free_regs   = 0;
	int i;
	int j;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (r->bytes < bytes)
			continue;

		for (j = 0; j < c->active_vars->size; j++) {
			scf_active_var_t* active    = c->active_vars->data[j];
			scf_dag_node_t*   dn_active = active->dag_node;

			if (dn_active->color < 0)
				continue;

			if (X64_COLOR_CONFLICT(r->color, dn_active->color))
				break;
		}

		if (j < c->active_vars->size)
			active_regs[nb_active_regs++] = r;
		else
			free_regs[nb_free_regs++]     = r;
	}

	if (nb_free_regs > 0) {
		int min = 0;
		scf_register_x64_t* r_min = NULL;

		for (i = 0; i < nb_free_regs; i++) {
			scf_register_x64_t*	r = free_regs[i];

			int nb_vars = x64_reg_cached_vars(r);

			if (!r_min) {
				r_min = r;
				min   = nb_vars;

			} else if (min > nb_vars) {
				r_min = r;
				min   = nb_vars;
			}
		}
		return r_min;
	}

	assert(nb_active_regs > 0);

	int min = 0;
	scf_register_x64_t* r_min = NULL;

	for (i = 0; i < nb_active_regs; i++) {
		scf_register_x64_t*	r = active_regs[i];

		int nb_vars = x64_reg_cached_vars(r);

		if (!r_min) {
			r_min = r;
			min   = nb_vars;

		} else if (min > r->dag_nodes->size) {
			r_min = r;
			min   = nb_vars;
		}
	}
	return r_min;
}

static int _x64_select_regs(scf_native_t* ctx, scf_basic_block_t* bb, scf_function_t* f)
{
	scf_graph_t* g = scf_graph_alloc();
	if (!g)
		return -ENOMEM;

	scf_vector_t* colors = NULL;
	scf_list_t*   l;

	int ret = 0;
	int i;

	for (i = 0; i < f->argv->size && i < X64_ABI_NB; i++) {
		scf_variable_t*     v = f->argv->data[i];

		scf_logw("v: %s\n", v->w->text->data);

		x64_rcg_node_t*     rn;
		scf_graph_node_t*   gn;
		scf_dag_node_t*     dn;
		scf_register_x64_t* r;

		for (l = scf_list_head(&bb->dag_list_head); l != scf_list_sentinel(&bb->dag_list_head);
				l = scf_list_next(l)) {
			dn = scf_list_data(l, scf_dag_node_t, list);

			if (dn->var == v) {
				r = x64_find_register_id_bytes(x64_abi_regs[i], v->size);
				dn->color = r->color;
				break;
			}
		}

		if (l == scf_list_sentinel(&bb->dag_list_head)) {
			dn = scf_dag_node_alloc(v->type, v);
			if (!dn) {
				scf_loge("\n");
				ret = -ENOMEM;
				goto error;
			}

			r = x64_find_register_id_bytes(x64_abi_regs[i], v->size);
			dn->color = r->color;
			scf_list_add_tail(&bb->dag_list_head, &dn->list);
		}

		rn = calloc(1, sizeof(x64_rcg_node_t));
		if (!rn) {
			scf_loge("\n");
			ret = -ENOMEM;
			goto error;
		}

		gn = scf_graph_node_alloc();
		if (!gn) {
			scf_loge("\n");
			free(rn);
			rn = NULL;
			ret = -ENOMEM;
			goto error;
		}

		rn->dag_node = dn;
		rn->reg      = r;
		rn->OpCode   = NULL;
		gn->data     = rn;
		gn->color    = r->color;

		ret = scf_graph_add_node(g, gn);
		if (ret < 0) {
			free(rn);
			scf_graph_node_free(gn);
			goto error;
		}
	}
	scf_logd("g->nodes->size: %d\n", g->nodes->size);

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		x64_rcg_handler_t* h = scf_x64_find_rcg_handler(c->op->type);
		if (!h) {
			scf_loge("3ac operator %s not supported\n", c->op->name);
			ret = -EINVAL;
			goto error;
		}

		scf_3ac_code_print(c, NULL);

		ret = h->func(ctx, c, g);
		if (ret < 0)
			goto error;
	}

	scf_logd("g->nodes->size: %d\n", g->nodes->size);

	colors = scf_vector_alloc();
	if (!colors) {
		ret = -ENOMEM;
		goto error;
	}

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		ret = scf_vector_add(colors, (void*)r->color);
		if (ret < 0)
			goto error;
	}

	ret = scf_x64_graph_kcolor(g, 8, colors);
	if (ret < 0)
		goto error;

	for (i = 0; i < g->nodes->size; i++) {
		scf_graph_node_t* gn = g->nodes->data[i];
		x64_rcg_node_t*   rn = gn->data;

		if (!rn->dag_node) {
			_x64_rcg_node_printf(rn);
			continue;
		}

		rn->dag_node->color = gn->color;

		if (gn->color > 0) {
			scf_register_x64_t* r = x64_find_register_color(gn->color);

			ret = scf_vector_add(r->dag_nodes, rn->dag_node);
			if (ret < 0)
				goto error;
		}

		_x64_rcg_node_printf(rn);
	}
	printf("\n");

error:
	if (colors)
		scf_vector_free(colors);

	scf_graph_free(g);
	g = NULL;
	return ret;
}

static int _x64_make_insts_for_list(scf_native_t* ctx, scf_list_t* h, int bb_offset)
{
	scf_list_t* l;
	int ret;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		x64_inst_handler_t* h = scf_x64_find_inst_handler(c->op->type);
		if (!h) {
			scf_loge("3ac operator '%s' not supported\n", c->op->name);
			return -EINVAL;
		}

		ret = h->func(ctx, c);
		if (ret < 0) {
			scf_3ac_code_print(c, NULL);
			scf_loge("3ac op '%s' make inst failed\n", c->op->name);
			return ret;
		}

		scf_3ac_code_print(c, NULL);
		_x64_inst_printf(c);

		if (!c->instructions)
			continue;

		int inst_bytes = 0;
		int i;
		for (i = 0; i < c->instructions->size; i++) {
			scf_instruction_t* inst = c->instructions->data[i];

			inst_bytes += inst->len;
		}
		c->inst_bytes  = inst_bytes;
		c->bb_offset   = bb_offset;
		bb_offset     += inst_bytes;
	}

	return bb_offset;
}

static void _x64_make_inst_for_jmps(scf_native_t* ctx, scf_function_t* f)
{
	int nb_jmps = f->jmps->size;
	while (nb_jmps > 0) {
		int i;
		for (i = 0; i < f->jmps->size; i++) {
			scf_3ac_code_t* c   = f->jmps->data[i];
			scf_3ac_code_t* dst = c->dst->code;

			scf_basic_block_t* cur_bb = c->basic_block;
			scf_basic_block_t* dst_bb = dst->basic_block;

			int32_t bytes = 0;
			scf_list_t* l = NULL;
			if (cur_bb->index < dst_bb->index) {

				for (l = scf_list_next(&cur_bb->list); l != &dst_bb->list; l = scf_list_next(l)) {

					scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

					if (bb->jmp_flag)
						break;
					bytes += bb->code_bytes;
				}
			} else {
				for (l = scf_list_prev(&cur_bb->list); l != &dst_bb->list; l = scf_list_prev(l)) {

					scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

					if (bb->jmp_flag)
						break;
					bytes += bb->code_bytes;
				}
			}

			if (l != &dst_bb->list)
				continue;

			assert(c->instructions && 1 == c->instructions->size);
			if (-128 <= bytes && bytes <= 127) {
				scf_instruction_t* inst = c->instructions->data[0];

				inst->code[1]  = (int8_t)bytes;
				inst->len      = 2;
				cur_bb->code_bytes -= 3; // default is 4 bytes
				c->inst_bytes      -= 3;
			} else {
				scf_instruction_t* inst = c->instructions->data[0];

				uint8_t* p = (uint8_t*)&bytes;
				int j;
				for (j = 0; j < 4; j++)
					inst->code[1 + j] = p[j];
			}
			cur_bb->jmp_flag = 0;
			nb_jmps--;
		}
	}
}

static void _x64_set_offset_for_relas(scf_native_t* ctx, scf_function_t* f, scf_vector_t* relas)
{
	scf_logw("relas->size: %d\n", relas->size);
	int i;
	for (i = 0; i < relas->size; i++) {
		scf_logw("relas->size: %d, i: %d\n", relas->size, i);

		scf_rela_t*        rela   = relas->data[i];
		scf_3ac_code_t*    c      = rela->code;
		scf_instruction_t* inst   = c->instructions->data[rela->inst_index];
		scf_basic_block_t* cur_bb = c->basic_block;

		scf_logw("relas->size: %d, i: %d, c: %p, index: %d\n", relas->size, i, c, rela->inst_index);
		scf_logw("cur_bb: %p\n", c->basic_block);

		scf_3ac_code_print(c, NULL);

		int bytes = 0;
		scf_list_t* l;
		for (l = scf_list_head(&f->basic_block_list_head); l != &cur_bb->list;
				l = scf_list_next(l)) {

			scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);
			bytes += bb->code_bytes;
		}

		bytes += c->bb_offset;

		int j;
		for (j = 0; j < rela->inst_index; j++) {
			scf_instruction_t* inst2 = c->instructions->data[j];
			bytes += inst2->len;
		}
		rela->inst_offset += bytes;
	}
}

int	_scf_x64_select_inst(scf_native_t* ctx)
{
	scf_x64_context_t*	x64 = ctx->priv;
	scf_function_t*     f   = x64->f;

	int ret = 0;
	scf_list_t* l;

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head);
			l = scf_list_next(l)) {

		scf_basic_block_t* bb = scf_list_data(l, scf_basic_block_t, list);

		ret = _x64_select_regs(ctx, bb, f);
		if (ret < 0)
			return ret;

		int bb_offset = 0;
#if 1
		ret = _x64_make_insts_for_list(ctx, &bb->load_list_head, bb_offset);
		if (ret < 0)
			return ret;
		bb_offset = ret;
#endif
		ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, bb_offset);
		if (ret < 0)
			return ret;
		bb_offset = ret;
#if 1
		ret = _x64_make_insts_for_list(ctx, &bb->save_list_head, bb_offset);
		if (ret < 0)
			return ret;
		bb_offset = ret;
#endif
		bb->code_bytes = bb_offset;
	}

	_x64_make_inst_for_jmps(ctx, f);
	scf_logw("\n");
	_x64_set_offset_for_relas(ctx, f, f->text_relas);

	scf_logw("\n");
	_x64_set_offset_for_relas(ctx, f, f->data_relas);
	scf_logw("\n");

	return 0;
}

static int _find_local_vars(scf_node_t* node, void* arg, scf_vector_t* results)
{
	scf_block_t* b = (scf_block_t*)node;

	if (SCF_OP_BLOCK == b->node.type && b->scope) {

		int i;
		for (i = 0; i < b->scope->vars->size; i++) {

			scf_variable_t* var = b->scope->vars->data[i];

			int ret = scf_vector_add(results, var);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_x64_select_inst(scf_native_t* ctx, scf_function_t* f)
{
	scf_x64_context_t* x64 = ctx->priv;

	x64->f = f;

	scf_vector_t* local_vars = scf_vector_alloc();
	if (!local_vars)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)f, NULL, local_vars, -1, _find_local_vars);
	if (ret < 0)
		return ret;

	// ABI: rdi rsi rdx rcx r8 r9
	int i;
	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t* v = f->argv->data[i];

		// arg0-arg5 passed by registers, alloc memory for them in stack.
		// from arg6, memory already alloced based on rbp by caller

		// after 'push %rbp', 'mov %rsp, %rbp', 'sub $local_var_size, %rsp'
		// stack status:
		// ...
		// arg7         : rbp + 24
		// arg6         : rbp + 16
		// ret          : rbp + 8
		// rbp (old rbp): rbp
		// local vars   :
		if (i < X64_ABI_NB) {
			ret = scf_vector_add(local_vars, v);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		} else
			v->bp_offset = 16 + (i - 6) * 8;
	}

	int local_vars_size = _x64_function_init(f, local_vars);
	if (local_vars_size < 0)
		return -1;

	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];
		assert(v->w);
		scf_logi("v: %p, name: %s, line: %d, pos: %d, size: %d, bp_offset: %d, arg_flag: %d\n",
				v, v->w->text->data, v->w->line, v->w->pos,
				v->size, v->bp_offset, v->arg_flag);
	}
	scf_logi("local_vars_size: %d\n", local_vars_size);

	ret = _scf_x64_select_inst(ctx);
	if (ret < 0)
		return ret;

	ret = _x64_function_finish(f);
	return ret;
}

int scf_x64_write_elf(scf_native_t* ctx, const char* path)
{
	scf_loge("\n");
	return -1;
}

scf_native_ops_t	native_ops_x64 = {
	.name				= "x64",

	.open				= scf_x64_open,
	.close				= scf_x64_close,

	.select_inst        = scf_x64_select_inst,
	.write_elf			= scf_x64_write_elf,
};

