#ifndef SCF_X64_OPCODE_H
#define SCF_X64_OPCODE_H

#include"scf_native.h"
#include"scf_x64_util.h"

typedef struct {
	int			type;

	char*		name;

	int			len;

	uint8_t		OpCodes[3];
	int			nb_OpCodes;

	// RegBytes only valid for immediate
	// same to OpBytes for E2G or G2E
	int			OpBytes;
	int			RegBytes;
	int			EG;

	uint8_t		ModRM_OpCode;
	int			ModRM_OpCode_used;

	int         nb_regs;
	uint32_t    regs[2];
} scf_x64_OpCode_t;

scf_x64_OpCode_t*   x64_find_OpCode_by_type(const int type);

scf_x64_OpCode_t*   x64_find_OpCode(const int type, const int OpBytes, const int RegBytes, const int EG);

int                 x64_find_OpCodes(scf_vector_t* results, const int type, const int OpBytes, const int RegBytes, const int EG);

#endif

