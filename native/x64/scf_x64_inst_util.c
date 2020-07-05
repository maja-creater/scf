#include"scf_x64.h"

static scf_instruction_t* _x64_make_OpCode(scf_x64_OpCode_t* OpCode, int dst_bytes)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == dst_bytes)
		inst->code[inst->len++] = SCF_X64_REX_INIT + SCF_X64_REX_W;
	else if (2 == dst_bytes)
		inst->code[inst->len++] = 0x66;

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

	if (SCF_X64_RM_EBP !=  base && SCF_X64_RM_ESP != base
			&& 0 == disp) {
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

scf_instruction_t* x64_make_inst_I(scf_x64_OpCode_t* OpCode, uint8_t* imm, int size)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	inst->OpCode = (scf_OpCode_t*)OpCode;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i];
	return inst;
}

scf_instruction_t* x64_make_inst_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	inst->OpCode = (scf_OpCode_t*)OpCode;
	assert(1 == OpCode->nb_OpCodes);

	inst->code[inst->len++] = OpCode->OpCodes[0] + r->id;
	return inst;
}

scf_instruction_t* x64_make_inst_I2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, uint8_t* imm, int size)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	size = size > r_dst->bytes ? r_dst->bytes : size;

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes)
		inst->code[inst->len++] = SCF_X64_REX_INIT + SCF_X64_REX_W;
	else if (2 == r_dst->bytes)
		inst->code[inst->len++] = 0x66;

	assert(1 == OpCode->nb_OpCodes);
	inst->code[inst->len++] = OpCode->OpCodes[0] + r_dst->id;

	int i;
	for (i = 0; i < size; i++)
		inst->code[inst->len++] = imm[i]; 
	return inst;
}

scf_instruction_t* x64_make_inst_E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return NULL;

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r->bytes)
		inst->code[inst->len++] = SCF_X64_REX_INIT + SCF_X64_REX_W;
	else if (2 == r->bytes)
		inst->code[inst->len++] = 0x66;

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

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

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, v->size);
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

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, v_dst->size);
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

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes);
	if (!inst)
		return NULL;

	if (_x64_make_disp(prela, inst, r_dst->id, base, offset) < 0) {
		free(inst);
		return NULL;
	}

	return inst;
}


scf_instruction_t* x64_make_inst_G2E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src)
{
	if (OpCode->ModRM_OpCode_used) {
		scf_loge("ModRM opcode invalid\n");
		return NULL;
	}

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes);
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

	scf_instruction_t* inst = _x64_make_OpCode(OpCode, r_dst->bytes);
	if (!inst)
		return NULL;

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_dst->id);
	scf_ModRM_setRM(&ModRM, r_src->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

	inst->code[inst->len++] = ModRM;
	return inst;
}

