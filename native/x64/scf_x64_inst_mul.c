#include"scf_x64.h"

static int _int_mul_src(scf_x64_OpCode_t* mul, scf_register_x64_t* rh, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	int size = src->var->size;

	scf_x64_OpCode_t*  mov;
	scf_instruction_t* inst;

	if (0 == src->color) {
		assert(scf_variable_const(src->var));

	    mov  = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, rh, (uint8_t*)&src->var->data, size);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_E(mul, rh);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M(&rela, mul, src->var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	}

	return 0;
}

int x64_inst_int_mul(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	assert(0 != dst->color);

	scf_register_x64_t* rs   = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;
	scf_rela_t*         rela = NULL;

	int size = src->var->size;
	int ret;

	scf_x64_OpCode_t* 	mov = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);
	scf_register_x64_t* rl  = x64_find_register_type_id_bytes(0, SCF_X64_REG_AX, size);
	scf_register_x64_t* rh;
	scf_x64_OpCode_t* 	mul;
	scf_x64_OpCode_t* 	mov2;

	if (1 == size)
		rh = x64_find_register_type_id_bytes(0, SCF_X64_REG_AH, size);
	else
		rh = x64_find_register_type_id_bytes(0, SCF_X64_REG_DX, size);

	if (scf_type_is_signed(src->var->type))
		mul = x64_find_OpCode(SCF_X64_IMUL, size, size, SCF_X64_E);
	else
		mul = x64_find_OpCode(SCF_X64_MUL,  size, size, SCF_X64_E);

	if (dst->color > 0) {
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);

		if (rd->id != rl->id) {
			ret = x64_overflow_reg(rl, c, f);
			if (ret < 0)
				return ret;
		}

		if (rd->id != rh->id) {
			ret = x64_overflow_reg(rh, c, f);
			if (ret < 0)
				return ret;
		}
	} else {
		ret = x64_overflow_reg(rl, c, f);
		if (ret < 0)
			return ret;

		ret = x64_overflow_reg(rh, c, f);
		if (ret < 0)
			return ret;
	}

	if (dst->color > 0) {
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 1);

		if (src->color > 0) {
			X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

			if (rd->id == rl->id) {
				inst = x64_make_inst_E(mul, rs);
				X64_INST_ADD_CHECK(c->instructions, inst);

			} else if (rs->id == rl->id) {
				inst = x64_make_inst_E(mul, rd);
				X64_INST_ADD_CHECK(c->instructions, inst);

			} else {
				inst = x64_make_inst_G2E(mov, rl, rd);
				X64_INST_ADD_CHECK(c->instructions, inst);

				inst = x64_make_inst_E(mul, rs);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}
		} else {
			if (rd->id != rl->id) {
				inst = x64_make_inst_G2E(mov, rl, rd);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}

			int ret = _int_mul_src(mul, rh, src, c, f);
			if (ret < 0)
				return ret;
		}
	} else {
		if (src->color > 0) {
			X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

			if (rs->id != rl->id) {
				inst = x64_make_inst_G2E(mov, rl, rs);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}

			inst = x64_make_inst_M(&rela, mul, dst->var, NULL);
			X64_INST_ADD_CHECK(c->instructions, inst);
			X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
		} else {

			mov2 = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_E2G);

			inst = x64_make_inst_M2G(&rela, mov2, rl, NULL, dst->var);
			X64_INST_ADD_CHECK(c->instructions, inst);
			X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);

			int ret = _int_mul_src(mul, rh, src, c, f);
			if (ret < 0)
				return ret;
		}
	}

	if (rd) {
		if (rd->id != rl->id) {
			inst = x64_make_inst_G2E(mov, rd, rl);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		inst = x64_make_inst_G2M(&rela, mov, dst->var, NULL, rl);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
	}

	return 0;
}

