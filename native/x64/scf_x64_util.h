#ifndef SCF_X64_UTIL_H
#define SCF_X64_UTIL_H

#include"scf_def.h"

enum scf_x64_REX_values {
	SCF_X64_REX_INIT	= 0x40,
	SCF_X64_REX_B		= 0x1,
	SCF_X64_REX_X		= 0x2,
	SCF_X64_REX_R		= 0x4,
	SCF_X64_REX_W		= 0x8,
};

enum scf_x64_OpCode_types {
	SCF_X64_MOV		= 0,

	SCF_X64_MOVSX,
	SCF_X64_MOVZX,

	SCF_X64_MOVS,
	SCF_X64_STOS,

	SCF_X64_LEA,

	SCF_X64_PUSH,
	SCF_X64_POP,

	SCF_X64_INC,
	SCF_X64_DEC,

	// 10
	SCF_X64_XOR,
	SCF_X64_AND,
	SCF_X64_OR,
	SCF_X64_NOT,

	SCF_X64_NEG,

	// 15
	SCF_X64_CALL,
	SCF_X64_RET,

	SCF_X64_ADD,
	SCF_X64_SUB,

	SCF_X64_MUL,
	SCF_X64_DIV,

	// 21
	SCF_X64_IMUL,
	SCF_X64_IDIV,

	// sign-extend ax to dx:ax
	SCF_X64_CBW,
	SCF_X64_CWD = SCF_X64_CBW,
	SCF_X64_CDQ = SCF_X64_CBW,
	SCF_X64_CQO = SCF_X64_CBW,

	// 24
	SCF_X64_SAR,
	SCF_X64_SHR,
	SCF_X64_SHL,

	SCF_X64_CMP,
	SCF_X64_TEST,

	SCF_X64_SETZ,
	SCF_X64_SETNZ,

	// 31
	SCF_X64_SETG,
	SCF_X64_SETGE,

	SCF_X64_SETL,
	SCF_X64_SETLE,

	SCF_X64_ADDSS,
	SCF_X64_ADDSD,

	SCF_X64_SUBSS,
	SCF_X64_SUBSD,

	SCF_X64_MULSS,
	SCF_X64_MULSD,

	// 41
	SCF_X64_DIVSS,
	SCF_X64_DIVSD,

	SCF_X64_MOVSS,
	SCF_X64_MOVSD,

	SCF_X64_UCOMISS,
	SCF_X64_UCOMISD,

	// 47
	SCF_X64_CVTSI2SD,
	SCF_X64_CVTSI2SS,

	SCF_X64_CVTSS2SD,
	SCF_X64_CVTSD2SS,

	SCF_X64_CVTTSD2SI,
	SCF_X64_CVTTSS2SI,

	SCF_X64_PXOR,

	SCF_X64_JZ,
	SCF_X64_JNZ,

	SCF_X64_JG,
	SCF_X64_JGE,

	SCF_X64_JL,
	SCF_X64_JLE,

	SCF_X64_JA,
	SCF_X64_JAE,

	SCF_X64_JB,
	SCF_X64_JBE,

	SCF_X64_JMP,

	SCF_X64_NB
};

enum scf_x64_Mods {
	SCF_X64_MOD_BASE		= 0x0, // 00, [base]
	SCF_X64_MOD_BASE_DISP8	= 0x1, // 01, [base] + disp8
	SCF_X64_MOD_BASE_DISP32	= 0x2, // 10, [base] + disp32
	SCF_X64_MOD_REGISTER	= 0x3, // 11, register
};

enum scf_x64_SIBs {
	SCF_X64_SIB_SCALE1      = 0x0, // 00, index * 1 + base
	SCF_X64_SIB_SCALE2      = 0x1, // 01, index * 2 + base
	SCF_X64_SIB_SCALE4      = 0x2, // 10, index * 4 + base
	SCF_X64_SIB_SCALE8      = 0x3, // 11, index * 8 + base
};

enum scf_x64_RMs {
	// others same to scf_x64_REGs

	// when Mod = 11
	SCF_X64_RM_ESP		= 0x4, // 100
	SCF_X64_RM_SP		= 0x4, // 100
	SCF_X64_RM_AH		= 0x4, // 100
	SCF_X64_RM_MM4		= 0x4, // 100
	SCF_X64_RM_XMM4		= 0x4, // 100

	SCF_X64_RM_SIB			= 0x4, // 100, when Mod = 00
	SCF_X64_RM_SIB_DISP8	= 0x4, // 100, when Mod = 01
	SCF_X64_RM_SIB_DISP32	= 0x4, // 100, when Mod = 10

	// when Mod = 11
	SCF_X64_RM_EBP		= 0x5, // 101
	SCF_X64_RM_BP		= 0x5, // 101
	SCF_X64_RM_CH		= 0x5, // 101
	SCF_X64_RM_MM5		= 0x5, // 101
	SCF_X64_RM_XMM5		= 0x5, // 101

	SCF_X64_RM_R12      = 0xc,
	SCF_X64_RM_R13      = 0xd,


	SCF_X64_RM_DISP32		= 0x5, // 101, when Mod = 00
	SCF_X64_RM_EBP_DISP8	= 0x5, // 101, when Mod = 01
	SCF_X64_RM_EBP_DISP32	= 0x5, // 101, when Mod = 10
};

enum scf_x64_REGs {
	SCF_X64_REG_AL		= 0x0,
	SCF_X64_REG_AX		= 0x0,
	SCF_X64_REG_EAX		= 0x0,
	SCF_X64_REG_RAX		= 0x0,
	SCF_X64_REG_MM0		= 0x0,
	SCF_X64_REG_XMM0	= 0x0,

	SCF_X64_REG_CL		= 0x1,
	SCF_X64_REG_CX		= 0x1,
	SCF_X64_REG_ECX		= 0x1,
	SCF_X64_REG_RCX		= 0x1,
	SCF_X64_REG_MM1		= 0x1,
	SCF_X64_REG_XMM1	= 0x1,

	SCF_X64_REG_DL		= 0x2,
	SCF_X64_REG_DX		= 0x2,
	SCF_X64_REG_EDX		= 0x2,
	SCF_X64_REG_RDX		= 0x2,
	SCF_X64_REG_MM2		= 0x2,
	SCF_X64_REG_XMM2	= 0x2,

	SCF_X64_REG_BL		= 0x3,
	SCF_X64_REG_BX		= 0x3,
	SCF_X64_REG_EBX		= 0x3,
	SCF_X64_REG_RBX		= 0x3,
	SCF_X64_REG_MM3		= 0x3,
	SCF_X64_REG_XMM3	= 0x3,

	SCF_X64_REG_AH		= 0x4,
	SCF_X64_REG_SP		= 0x4,
	SCF_X64_REG_ESP		= 0x4,
	SCF_X64_REG_RSP		= 0x4,
	SCF_X64_REG_MM4		= 0x4,
	SCF_X64_REG_XMM4	= 0x4,

	SCF_X64_REG_CH		= 0x5,
	SCF_X64_REG_BP		= 0x5,
	SCF_X64_REG_EBP		= 0x5,
	SCF_X64_REG_RBP		= 0x5,
	SCF_X64_REG_MM5		= 0x5,
	SCF_X64_REG_XMM5	= 0x5,

	SCF_X64_REG_DH		= 0x6,
	SCF_X64_REG_SIL     = 0x6,
	SCF_X64_REG_SI		= 0x6,
	SCF_X64_REG_ESI		= 0x6,
	SCF_X64_REG_RSI		= 0x6,
	SCF_X64_REG_MM6		= 0x6,
	SCF_X64_REG_XMM6	= 0x6,

	SCF_X64_REG_BH		= 0x7,
	SCF_X64_REG_DIL     = 0x7,
	SCF_X64_REG_DI		= 0x7,
	SCF_X64_REG_EDI		= 0x7,
	SCF_X64_REG_RDI		= 0x7,
	SCF_X64_REG_MM7		= 0x7,
	SCF_X64_REG_XMM7	= 0x7,

	SCF_X64_REG_R8D     = 0x8,
	SCF_X64_REG_R8      = 0x8,

	SCF_X64_REG_R9D     = 0x9,
	SCF_X64_REG_R9      = 0x9,

	SCF_X64_REG_R10D    = 0xa,
	SCF_X64_REG_R10     = 0xa,

	SCF_X64_REG_R11D    = 0xb,
	SCF_X64_REG_R11     = 0xb,

	SCF_X64_REG_R12D    = 0xc,
	SCF_X64_REG_R12     = 0xc,

	SCF_X64_REG_R13D    = 0xd,
	SCF_X64_REG_R13     = 0xd,

	SCF_X64_REG_R14D    = 0xe,
	SCF_X64_REG_R14     = 0xe,

	SCF_X64_REG_R15D    = 0xf,
	SCF_X64_REG_R15     = 0xf,
};

enum scf_x64_EG_types {
	SCF_X64_G	= 0,
	SCF_X64_I	= 1,
	SCF_X64_G2E	= 2,
	SCF_X64_E2G = 3,
	SCF_X64_I2E = 4,
	SCF_X64_I2G = 5,
	SCF_X64_E	= 6,
};

static inline uint8_t scf_ModRM_getMod(uint8_t ModRM)
{
	return ModRM >> 6;
}

static inline uint8_t scf_ModRM_getReg(uint8_t ModRM)
{
	return (ModRM >> 3) & 0x7;
}

static inline uint8_t scf_ModRM_getRM(uint8_t ModRM)
{
	return ModRM & 0x7;
}

static inline void scf_ModRM_setMod(uint8_t* ModRM, uint8_t v)
{
	*ModRM |= (v << 6);
}

static inline void scf_ModRM_setReg(uint8_t* ModRM, uint8_t v)
{
	*ModRM |= (v & 0x7) << 3;
}

static inline void scf_ModRM_setRM(uint8_t* ModRM, uint8_t v)
{
	*ModRM |= v & 0x7;
}

static inline uint8_t scf_SIB_getScale(uint8_t SIB)
{
	return (SIB >> 6) & 0x3;
}
static inline void scf_SIB_setScale(uint8_t* SIB, uint8_t scale)
{
	*SIB |= scale << 6;
}

static inline uint8_t scf_SIB_getIndex(uint8_t SIB)
{
	return (SIB >> 3) & 0x7;
}
static inline void scf_SIB_setIndex(uint8_t* SIB, uint8_t index)
{
	*SIB |= (index & 0x7) << 3;
}

static inline uint8_t scf_SIB_getBase(uint8_t SIB)
{
	return SIB & 0x7;
}
static inline void scf_SIB_setBase(uint8_t* SIB, uint8_t base)
{
	*SIB |= base & 0x7;
}

#endif

