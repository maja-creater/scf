#include"scf_native_x64.h"

scf_instruction_t* _x64_make_instruction_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	assert(1 == OpCode->nb_OpCodes);

	if (OpCode->ModRM_OpCode_used) {
		uint8_t ModRM = 0;
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);
		scf_ModRM_setRM(&ModRM, r->id);

		inst->code[inst->len++] = OpCode->OpCodes[0];
		inst->code[inst->len++] = ModRM;
	} else {
		inst->code[inst->len++] = OpCode->OpCodes[0] + r->id;
	}
	return inst;
}

scf_instruction_t* _x64_make_instruction_E_reg(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_src)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_src->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);
	scf_ModRM_setRM(&ModRM, r_src->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

	inst->code[inst->len++] = ModRM;

	return inst;
}

scf_instruction_t* _x64_make_instruction_E_var(scf_x64_OpCode_t* OpCode, scf_variable_t* v_src)
{
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == v_src->size) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);

	if (v_src->bp_offset < 127 && v_src->bp_offset > -128) {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP8);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);

		inst->code[inst->len++] = ModRM;
		inst->code[inst->len++] = (int8_t)v_src->bp_offset;
	} else {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP32);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);

		inst->code[inst->len++] = ModRM;

		uint8_t* disp = (uint8_t*)&v_src->bp_offset;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = disp[i];
	}

	return inst;
}

scf_instruction_t* _x64_make_instruction_I2E_reg_imm32(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, int32_t imm)
{
	assert(r_dst->bytes >= 4);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);
	scf_ModRM_setRM(&ModRM, r_dst->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);
	inst->code[inst->len++] = ModRM;

	uint8_t* data = (uint8_t*)&imm;
	for (i = 0; i < sizeof(imm); i++)
		inst->code[inst->len++] = data[i];
	return inst;
}

scf_instruction_t* _x64_make_instruction_I2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src)
{
	assert(v_src->size == r_dst->bytes);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	assert(1 == OpCode->nb_OpCodes);
	inst->code[inst->len++] = OpCode->OpCodes[0] + r_dst->id;

	uint8_t* data = (uint8_t*)&v_src->data;
	int i;
	for (i = 0; i < v_src->size; i++)
		inst->code[inst->len++] = data[i]; 
	return inst;
}

scf_instruction_t* _x64_make_instruction_I2E_reg(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src)
{
	assert(v_src->size <= r_dst->bytes);
	assert(v_src->size <= 4);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);
	scf_ModRM_setRM(&ModRM, r_dst->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);
	inst->code[inst->len++] = ModRM;

	uint8_t* data = (uint8_t*)&v_src->data;
	for (i = 0; i < v_src->size; i++)
		inst->code[inst->len++] = data[i]; 
	return inst;
}

scf_instruction_t* _x64_make_instruction_I2E_var(scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_variable_t* v_src)
{
	assert(v_src->size <= v_dst->size);
	assert(v_src->size <= 4);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == v_dst->size) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	if (OpCode->ModRM_OpCode_used)
		scf_ModRM_setReg(&ModRM, OpCode->ModRM_OpCode);

	if (v_dst->bp_offset < 127 && v_dst->bp_offset > -128) {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP8);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);

		inst->code[inst->len++] = ModRM;
		inst->code[inst->len++] = (int8_t)v_dst->bp_offset;
	} else {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP32);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);

		inst->code[inst->len++] = ModRM;

		uint8_t* disp = (uint8_t*)&v_dst->bp_offset;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = disp[i];
	}

	uint8_t* data = (uint8_t*)&v_src->data;
	for (i = 0; i < v_src->size; i++)
		inst->code[inst->len++] = data[i]; 
	return inst;
}

scf_instruction_t* _x64_make_instruction_G2E(scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_register_x64_t* r_src)
{
//	assert(r_src->bytes == v_dst->size);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == v_dst->size) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_src->id);

	if (v_dst->bp_offset < 127 && v_dst->bp_offset > -128) {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP8);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);

		inst->code[inst->len++] = ModRM;
		inst->code[inst->len++] = (int8_t)v_dst->bp_offset;
	} else {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP32);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);

		inst->code[inst->len++] = ModRM;

		uint8_t* disp = (uint8_t*)&v_dst->bp_offset;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = disp[i];
	}

	return inst;
}

scf_instruction_t* _x64_make_instruction_E2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src)
{
	assert(v_src->size == r_dst->bytes);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_dst->id);

	if (v_src->bp_offset < 127 && v_src->bp_offset > -128) {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP8);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP8);

		inst->code[inst->len++] = ModRM;
		inst->code[inst->len++] = (int8_t)v_src->bp_offset;
	} else {
		scf_ModRM_setRM(&ModRM, SCF_X64_RM_EBP_DISP32);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_BASE_DISP32);

		inst->code[inst->len++] = ModRM;

		uint8_t* disp = (uint8_t*)&v_src->bp_offset;
		for (i = 0; i < 4; i++)
			inst->code[inst->len++] = disp[i];
	}

	return inst;
}

scf_instruction_t* _x64_make_instruction_G2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src)
{
//	assert(r_src->bytes == r_dst->bytes);

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	inst->OpCode = (scf_OpCode_t*)OpCode;

	if (8 == r_dst->bytes) {
		uint8_t REX = SCF_X64_REX_INIT + SCF_X64_REX_W;
		inst->code[inst->len++] = REX;
	}

	int i;
	for (i = 0; i < OpCode->nb_OpCodes; i++)
		inst->code[inst->len++] = OpCode->OpCodes[i];

	uint8_t ModRM = 0;
	scf_ModRM_setReg(&ModRM, r_src->id);
	scf_ModRM_setRM(&ModRM, r_dst->id);
	scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);
	inst->code[inst->len++] = ModRM;

	return inst;
}

