#ifndef SCF_NATIVE_X64_UTIL_H
#define SCF_NATIVE_X64_UTIL_H

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

	SCF_X64_PUSH,
	SCF_X64_POP,

	SCF_X64_INC,
	SCF_X64_DEC,

	SCF_X64_XOR,

	SCF_X64_CALL,
	SCF_X64_RET,

	SCF_X64_ADD,
	SCF_X64_SUB,

	SCF_X64_MUL,
	SCF_X64_DIV,

	SCF_X64_CMP,

	SCF_X64_SETZ,
	SCF_X64_SETNZ,

	SCF_X64_SETG,
	SCF_X64_SETGE,

	SCF_X64_SETL,
	SCF_X64_SETLE,

	SCF_X64_JZ,
	SCF_X64_JNZ,
	SCF_X64_JMP,

	SCF_X64_NB
};

enum scf_x64_Mods {
	SCF_X64_MOD_BASE		= 0x0, // 00, [base]
	SCF_X64_MOD_BASE_DISP8	= 0x1, // 01, [base] + disp8
	SCF_X64_MOD_BASE_DISP32	= 0x2, // 10, [base] + disp32
	SCF_X64_MOD_REGISTER	= 0x3, // 11, register
};

enum scf_x64_RMs {
	SCF_X64_RM_EAX		= 0x0, // 000
	SCF_X64_RM_AX		= 0x0, // 000
	SCF_X64_RM_AL		= 0x0, // 000
	SCF_X64_RM_MM0		= 0x0, // 000
	SCF_X64_RM_XMM0		= 0x0, // 000

	SCF_X64_RM_ECX		= 0x1, // 001
	SCF_X64_RM_CX		= 0x1, // 001
	SCF_X64_RM_CL		= 0x1, // 001
	SCF_X64_RM_MM1		= 0x1, // 001
	SCF_X64_RM_XMM1		= 0x1, // 001

	SCF_X64_RM_EDX		= 0x2, // 010
	SCF_X64_RM_DX		= 0x2, // 010
	SCF_X64_RM_DL		= 0x2, // 010
	SCF_X64_RM_MM2		= 0x2, // 010
	SCF_X64_RM_XMM2		= 0x2, // 010

	SCF_X64_RM_EBX		= 0x3, // 011
	SCF_X64_RM_BX		= 0x3, // 011
	SCF_X64_RM_BL		= 0x3, // 011
	SCF_X64_RM_MM3		= 0x3, // 011
	SCF_X64_RM_XMM3		= 0x3, // 011

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

	SCF_X64_RM_DISP32		= 0x5, // 101, when Mod = 00
	SCF_X64_RM_EBP_DISP8	= 0x5, // 101, when Mod = 01
	SCF_X64_RM_EBP_DISP32	= 0x5, // 101, when Mod = 10

	SCF_X64_RM_ESI		= 0x6, // 110
	SCF_X64_RM_SI		= 0x6, // 110
	SCF_X64_RM_DH		= 0x6, // 110
	SCF_X64_RM_MM6		= 0x6, // 110
	SCF_X64_RM_XMM6		= 0x6, // 110

	SCF_X64_RM_EDI		= 0x7, // 111
	SCF_X64_RM_DI		= 0x7, // 111
	SCF_X64_RM_BH		= 0x7, // 111
	SCF_X64_RM_MM7		= 0x7, // 111
	SCF_X64_RM_XMM7		= 0x7, // 111
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
	SCF_X64_REG_SI		= 0x6,
	SCF_X64_REG_ESI		= 0x6,
	SCF_X64_REG_RSI		= 0x6,
	SCF_X64_REG_MM6		= 0x6,
	SCF_X64_REG_XMM6	= 0x6,

	SCF_X64_REG_BH		= 0x7,
	SCF_X64_REG_DI		= 0x7,
	SCF_X64_REG_EDI		= 0x7,
	SCF_X64_REG_RDI		= 0x7,
	SCF_X64_REG_MM7		= 0x7,
	SCF_X64_REG_XMM7	= 0x7,
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
	*ModRM |= (v << 3);
}

static inline void scf_ModRM_setRM(uint8_t* ModRM, uint8_t v)
{
	*ModRM |= v;
}

static inline uint8_t scf_SIB_getScale(uint8_t SIB)
{
	return (SIB >> 6) << 1;
}
static inline void scf_SIB_setScale(uint8_t* SIB, uint8_t scale)
{
	*SIB |= (scale >> 1) << 6;
}

static inline uint8_t scf_SIB_getIndex(uint8_t SIB)
{
	return (SIB >> 3) & 0x7;
}
static inline void scf_SIB_setIndex(uint8_t* SIB, uint8_t index)
{
	*SIB |= (index << 3);
}

static inline uint8_t scf_SIB_getBase(uint8_t SIB)
{
	return SIB & 0x7;
}
static inline void scf_SIB_setBase(uint8_t* SIB, uint8_t base)
{
	*SIB |= base;
}

#endif

