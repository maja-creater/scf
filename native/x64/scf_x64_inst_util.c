#include"scf_x64.h"

static scf_instruction_t* _x64_make_OpCode(scf_x64_OpCode_t* OpCode, int bytes, scf_register_x64_t* r0, scf_register_x64_t* r1)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	if (8 == bytes) {
		switch (OpCode->type) {
			case SCF_X64_MOVSD:
			case SCF_X64_ADDSD:
			case SCF_X64_SUBSD:
			case SCF_X64_MULSD:
			case SCF_X64_DIVSD:
			case SCF_X64_PUSH:
			case SCF_X64_POP:
			case SCF_X64_RET:
			case SCF_X64_CALL:
				break;
			default:
				inst->code[inst->len++] = SCF_X64_REX_INIT + SCF_X64_REX_W;
				break;
		};
	} else if (2 == bytes) {
		inst->code[inst->len++] = 0x66;

	} else if (1 == bytes) {
		if (r0 || r1) {
			uint8_t prefix = 0;

			if (r0) {
				switch (X64_COLOR_ID(r0->color)) {
					case SCF_X64_REG_ESI:
					case SCF_X64_REG_EDI:
						prefix = SCF_X64_REX_INIT;
						break;
					default:
						break;
				};
			}

			if (r1) {
				switch (X64_COLOR_ID(r1->color)) {
					case SCF_X64_REG_ESI:
					case SCF_X64_REG_EDI:
						prefix = SCF_X64_REX_INIT;
						break;
					default:
						break;
				};
			}

			if (prefix)
				inst->code[inst->len++] = prefix;
		}
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	return inst;
}

static int _x64_make_disp(scf_rela_t** prela, scf_instruction_t* inst, uint32_t reg, uint32_t base, int32_t disp)
{
	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, reg);

	// global var, use the offset to current instruction, such as 'mov 0(%rip), %rax'
	if (-1 == base) {
		scf_ModRM_setRM(&ModRM,  0x5);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE);
		inst->code[inst->len++] = ModRM;

		scf_rela_t* rela = calloc(1, sizeof(scf_rela_t));
		if (!rela)
			return -ENOMEM;

		rela->inst_offset = inst->len;
		*prela = rela;

		uint8_t* p = (uint8_t*)&disp;
		int i;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = p[i];

		scf_logw("global var\n");
		return 0;
	}

	scf_ModRM_setRM(&ModRM,  base);

	if (SCF_X64_RM_EBP !=  base && SCF_X64_RM_ESP != base && 0 == disp) {
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE);
		inst->code[inst->len++] = ModRM;
		return 0;
	}

	if (disp < 127 && disp > -128) {
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);
		inst->code[inst->len++] = ModRM;
	} else {
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);
		inst->code[inst->len++] = ModRM;
	}

	if (SCF_X64_RM_ESP ==  base) {
		uint8_t SIB = 0;
		scf_SIB_setBase(&SIB, base);
		scf_SIB_setIndex(&SIB, base);
		scf_SIB_setScale(&SIB, SCF_X64_SIB_SCALE1);
		inst->code[inst->len++] = SIB;
	}

	if (disp < 127 && disp > -128) {
		inst->code[inst->len++] = (int8_t)disp;
	} else {
		uint8_t* p = (uint8_t*)&disp;
		int i;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = p[i];
	}
	return 0;
}

void x64_make_inst_I2(scf_instruction_t* inst, scf_x64_OpCode_t* OpCode, uint8_t* imm, int size)
{
	inst->OpCode = (scf_OpCode_t*)OpCode;
	inst->len    = 0;

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];
}

scf_instruction_t* x64_make_inst_I(scf_x64_OpCode_t* OpCode, uint8_t* imm, int size)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	x64_make_inst_I2(inst, OpCode, imm, size);
	return inst;
}

scf_instruction_t* x64_make_inst(scf_x64_OpCode_t* OpCode, int size)
{
	return _x64_make_OpCode(OpCode, size, NULL, NULL);
}

scf_instruction_t* x64_make_inst_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r)
{
	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r->bytes, r, NULL);
	if (!inst)
		return NULL;

	inst->OpCode = (scf_OpCode_t*)OpCode;
	assert(1 == OpCode->nb_OpCodes);

	inst->code[inst->len - 1] += r->id;
	return inst;
}

scf_instruction_t* x64_make_inst_I2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, uint8_t* imm, int size)
{
	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, NULL);
	if (!inst)
		return NULL;

	assert(1 == OpCode->nb_OpCodes);
	inst->code[inst->len - 1] += r_dst->id;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i]; 
	return inst;
}

scf_instruction_t* x64_make_inst_E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r)
{
	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r->bytes, r, NULL);
	if (!inst)
		return NULL;

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);

	scf_ModRM_setRM(&ModRM, r->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

	inst->code[inst->len++] = ModRM;
	return inst;
}

scf_instruction_t* x64_make_inst_I2E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, uint8_t* imm, int size)
{
	scf_instruction_t* inst = x64_make_inst_E(OpCode, r_dst);
	if (!inst)
		return NULL;

	size = size > r_dst->bytes ? r_dst->bytes : size;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];
	return inst;
}

scf_instruction_t* x64_make_inst_M(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v, scf_register_x64_t* r_base)
{
	uint32_t base;
	int32_t  offset;
	uint8_t  reg = 0;

	if (OpCode->ModRM_OpCode_used)
		reg = OpCode->ModRM_OpCode;

	if (!r_base) {
		if (v->local_flag) {
			base   = SCF_X64_REG_RBP;
			offset = v->bp_offset;

		} else if (v->global_flag) {
			base   = -1;
			offset = 0;
		} else {
			scf_loge("temp var should give a register\n");
			return NULL;
		}
	} else {
		base = r_base->id;

		if (v->local_flag)
			offset = v->bp_offset;
		else
			offset = v->offset;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, v->size, NULL, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(prela, inst, reg, base, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_I2M(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_register_x64_t* r_base, uint8_t* imm, int32_t size)
{
	scf_instruction_t* inst = x64_make_inst_M(prela, OpCode, v_dst, r_base);
	if (!inst)
		return NULL;

	size = size > v_dst->size ? v_dst->size : size;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];
	return inst;
}

scf_instruction_t* x64_make_inst_G2M(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_register_x64_t* r_base, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	uint32_t base;
	int32_t  offset;

	if (!r_base) {
		if (v_dst->local_flag) {
			base   = SCF_X64_REG_RBP;
			offset = v_dst->bp_offset;

		} else if (v_dst->global_flag) {
			base   = -1;
			offset = 0;
		} else {
			scf_loge("temp var should give a register\n");
			return NULL;
		}
	} else {
		base = r_base->id;

		if (v_dst->local_flag)
			offset = v_dst->bp_offset;
		else
			offset = v_dst->offset;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, v_dst->size, r_src, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(prela, inst, r_src->id, base, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_M2G(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_base, scf_variable_t* v_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	uint32_t base;
	int32_t  offset;

	if (!r_base) {
		if (v_src->local_flag) {
			base   = SCF_X64_REG_RBP;
			offset = v_src->bp_offset;

		} else if (v_src->global_flag) {
			base   = -1;
			offset = 0;
		} else {
			scf_loge("temp var should give a register\n");
			return NULL;
		}
	} else {
		base = r_base->id;

		if (v_src->local_flag)
			offset = v_src->bp_offset;
		else
			offset = v_src->offset;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(prela, inst, r_dst->id, base, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_P2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_base, int32_t offset)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(NULL, inst, r_dst->id, r_base->id, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_G2P(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_src->bytes, r_src, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(NULL, inst, r_src->id, r_base->id, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_P(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, int size)
{
	uint8_t reg = 0;

	if (OpCode->ModRM_OpCode_used)
		reg = OpCode->ModRM_OpCode;

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, size, NULL, NULL);
	if (!inst)
		return NULL;

	if (_x64_make_disp(NULL, inst, reg, r_base->id, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}

scf_instruction_t* x64_make_inst_I2P(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, uint8_t* imm, int size)
{
	scf_instruction_t* inst = x64_make_inst_P(OpCode, r_base, offset, size);
	if (!inst)
		return NULL;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];
	return inst;
}

scf_instruction_t* x64_make_inst_G2E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, r_src);
	if (!inst)
		return NULL;

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_src->id);
	scf_ModRM_setRM(&ModRM, r_dst->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

	inst->code[inst->len++] = ModRM;
	return inst;
}

scf_instruction_t* x64_make_inst_E2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, r_src);
	if (!inst)
		return NULL;

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_dst->id);
	scf_ModRM_setRM(&ModRM, r_src->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

	inst->code[inst->len++] = ModRM;
	return inst;
}

scf_instruction_t* _x64_make_inst_SIB(scf_instruction_t* inst, scf_x64_OpCode_t* OpCode, uint32_t reg,  scf_register_x64_t* r_base,  scf_register_x64_t* r_index, int32_t scale, int32_t disp, int size)
{
	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, reg);
	scf_ModRM_setRM(&ModRM,  SCF_X64_RM_SIB);

	if (SCF_X64_RM_EBP !=  r_base->id && SCF_X64_RM_ESP != r_base->id && 0 == disp)
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE);
	else {
		if (disp < 127 && disp > -128)
			scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);
		else
			scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);
	}
	inst->code[inst->len++] = ModRM;

	uint8_t SIB = 0;
	scf_SIB_setBase(&SIB,  r_base->id);
	scf_SIB_setIndex(&SIB, r_index->id);
	switch (scale) {
		case 1:
			scf_SIB_setScale(&SIB, SCF_X64_SIB_SCALE1);
			break;
		case 2:
			scf_SIB_setScale(&SIB, SCF_X64_SIB_SCALE2);
			break;
		case 4:
			scf_SIB_setScale(&SIB, SCF_X64_SIB_SCALE4);
			break;
		case 8:
			scf_SIB_setScale(&SIB, SCF_X64_SIB_SCALE8);
			break;
		default:
			free(inst);
			return NULL;
			break;
	};
	inst->code[inst->len++] = SIB;

	if (SCF_X64_RM_EBP ==  r_base->id || SCF_X64_RM_ESP == r_base->id || 0 != disp) {

		if (disp <= 127 && disp >= -128)
			inst->code[inst->len++] = (int8_t)disp;
		else {
			uint8_t* p = (uint8_t*)&disp;
			int i;
			for (i = 0; i < 4; i++)
				inst->code[inst->len++] = p[i];
		}
	}
	return inst;
}

scf_instruction_t* x64_make_inst_SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base,  scf_register_x64_t* r_index, int32_t scale, int32_t disp, int size)
{
	scf_instruction_t* inst = _x64_make_OpCode(OpCode, size, NULL, NULL);
	if (!inst)
		return NULL;

	uint32_t reg = 0;
	if (OpCode->ModRM_OpCode_used)
		reg = OpCode->ModRM_OpCode;

	return _x64_make_inst_SIB(inst, OpCode, reg, r_base, r_index, scale, disp, size);
}

scf_instruction_t* x64_make_inst_SIB2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst,  scf_register_x64_t* r_base,  scf_register_x64_t* r_index, int32_t scale, int32_t disp)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes, r_dst, NULL);
	if (!inst)
		return NULL;

	return _x64_make_inst_SIB(inst, OpCode, r_dst->id, r_base, r_index, scale, disp, r_dst->bytes);
}

scf_instruction_t* x64_make_inst_G2SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, scf_register_x64_t* r_index, int32_t scale, int32_t disp, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_src->bytes, r_src, NULL);
	if (!inst)
		return NULL;

	return _x64_make_inst_SIB(inst, OpCode, r_src->id, r_base, r_index, scale, disp, r_src->bytes);
}

scf_instruction_t* x64_make_inst_I2SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, scf_register_x64_t* r_index, int32_t scale, int32_t disp, uint8_t* imm, int32_t size)
{
	uint32_t reg = 0;
	if (OpCode->ModRM_OpCode_used)
		reg = OpCode->ModRM_OpCode;

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, size, NULL, NULL);
	if (!inst)
		return NULL;

	inst = _x64_make_inst_SIB(inst, OpCode, reg, r_base, r_index, scale, disp, size);
	if (!inst)
		return NULL;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];

	return inst;
}
