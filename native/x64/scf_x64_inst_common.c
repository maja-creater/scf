#include"scf_x64.h"

int x64_float_OpCode_type(int OpCode_type, int var_type)
{
	if (SCF_VAR_FLOAT == var_type) {

		switch (OpCode_type) {
			case SCF_X64_MOV:
				return SCF_X64_MOVSS;
				break;

			case SCF_X64_ADD:
				return SCF_X64_ADDSS;
				break;
			case SCF_X64_SUB:
				return SCF_X64_SUBSS;
				break;
			case SCF_X64_MUL:
				return SCF_X64_MULSS;
				break;
			case SCF_X64_DIV:
				return SCF_X64_DIVSS;
				break;

			default:
				break;
		};
	} else {
		switch (OpCode_type) {
			case SCF_X64_MOV:
				return SCF_X64_MOVSD;
				break;

			case SCF_X64_ADD:
				return SCF_X64_ADDSD;
				break;
			case SCF_X64_SUB:
				return SCF_X64_SUBSD;
				break;
			case SCF_X64_MUL:
				return SCF_X64_MULSD;
				break;
			case SCF_X64_DIV:
				return SCF_X64_DIVSD;
				break;

			default:
				break;
		};
	}

	return -1;
}

static int _x64_inst_op2_imm(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t*   OpCode;
	scf_instruction_t*  inst;
	scf_register_x64_t* rd   = NULL;
	scf_register_x64_t* rs   = NULL;
	scf_rela_t*         rela = NULL;

	assert( scf_variable_const(src->var));
	assert(!scf_variable_float(src->var));

	int src_size = x64_variable_size(src->var);
	int dst_size = x64_variable_size(dst->var);

	src_size = src_size > dst_size ? dst_size : src_size;

	if (dst->color < 0
			&& 0 == dst->var->bp_offset
			&& dst->var->tmp_flag) {

		if (SCF_X64_MOV != OpCode_type) {
			scf_loge("\n");
			return -1;
		}

		X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);
	}

	if (dst->color > 0) {

		if (SCF_X64_MOV == OpCode_type)
			X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);
		else
			X64_SELECT_REG_CHECK(&rd, dst, c, f, 1);

		OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_I2G);
		if (OpCode) {
			inst = x64_make_inst_I2G(OpCode, rd, (uint8_t*)&src->var->data, src_size);
			X64_INST_ADD_CHECK(c->instructions, inst);
			return 0;
		}

		OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_I2E);
		if (OpCode) {
			inst = x64_make_inst_I2E(OpCode, rd, (uint8_t*)&src->var->data, src_size);
			X64_INST_ADD_CHECK(c->instructions, inst);
			return 0;
		}

		OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_G2E);
		if (!OpCode) {
			scf_loge("\n");
			return -EINVAL;
		}

		src->color = -1;
		src->var->tmp_flag = 1;
		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

		inst = x64_make_inst_G2E(OpCode, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

		src->color  = 0;
		src->loaded = 0;
		src->var->tmp_flag = 0;
		assert(0 == scf_vector_del(rs->dag_nodes, src));
		return 0;
	}

	OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_I2E);
	if (OpCode) {
		inst = x64_make_inst_I2M(&rela, OpCode, dst->var, NULL, (uint8_t*)&src->var->data, src_size);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
		return 0;
	}

	OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_G2E);
	if (!OpCode) {
		scf_loge("\n");
		return -EINVAL;
	}

	src->color = -1;
	src->var->tmp_flag = 1;
	X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

	inst = x64_make_inst_G2M(&rela, OpCode, dst->var, NULL, rs);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);

	src->color  = 0;
	src->loaded = 0;
	src->var->tmp_flag = 0;
	assert(0 == scf_vector_del(rs->dag_nodes, src));
	return 0;
}

static int _x64_inst_op2_imm_str(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	if (SCF_X64_MOV != OpCode_type) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;
	scf_x64_OpCode_t*   lea  = x64_find_OpCode(SCF_X64_LEA, 8, 8, SCF_X64_E2G);
	scf_rela_t*         rela = NULL;

	int size0 = x64_variable_size(dst->var);
	int size1 = x64_variable_size(src->var);

	assert(8 == size0);
	assert(8 == size1);

	X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);

	src->var->global_flag = 1;
	src->var->local_flag  = 0;
	src->var->tmp_flag    = 0;

	inst = x64_make_inst_M2G(&rela, lea, rd, NULL, src->var);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	return 0;
}

int x64_inst_op2(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	assert(0 != dst->color);

	scf_x64_OpCode_t*   OpCode = NULL;
	scf_register_x64_t* rs     = NULL;
	scf_register_x64_t* rd     = NULL;
	scf_instruction_t*  inst   = NULL;
	scf_rela_t*         rela   = NULL;

	if (0 == src->color) {

		if (scf_variable_const_string(src->var))
			return _x64_inst_op2_imm_str(OpCode_type, dst, src, c, f);

		if (!scf_variable_float(src->var)) {
			scf_loge("\n");
			return _x64_inst_op2_imm(OpCode_type, dst, src, c, f);
		}
		src->color = -1;
		src->var->global_flag = 1;
	}

	int src_size = x64_variable_size(src->var);
	int dst_size = x64_variable_size(dst->var);

	src_size = src_size > dst_size ? dst_size : src_size;

	if (SCF_X64_MOV          == OpCode_type
			|| SCF_X64_MOVSS == OpCode_type
			|| SCF_X64_MOVSD == OpCode_type)
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);
	else
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 1);

	if (src->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

		if (SCF_X64_MOV == OpCode && src->color == dst->color)
			return 0;

		OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_G2E);
		if (OpCode) {
			inst = x64_make_inst_G2E(OpCode, rd, rs);
			X64_INST_ADD_CHECK(c->instructions, inst);

		} else {
			OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_E2G);
			if (!OpCode) {
				scf_loge("OpCode_type: %d, size: %d, size: %d\n", OpCode_type, src_size, dst_size);
				return -EINVAL;
			}

			inst = x64_make_inst_E2G(OpCode, rd, rs);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		OpCode = x64_find_OpCode(OpCode_type, src_size, dst_size, SCF_X64_E2G);
		if (!OpCode) {
			scf_loge("OpCode_type: %d, size: %d, size: %d\n", OpCode_type, src_size, dst_size);
			return -EINVAL;
		}

		inst = x64_make_inst_M2G(&rela, OpCode, rd, NULL, src->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	}

	return 0;
}

int x64_inst_movx(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t*   movx;
	scf_x64_OpCode_t*   mov;
	scf_x64_OpCode_t*   push;
	scf_x64_OpCode_t*   pop;
	scf_x64_OpCode_t*   xor;

	scf_instruction_t*  inst;
	scf_register_x64_t* rs;
	scf_register_x64_t* rd = NULL;

	X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);

	if (scf_type_is_signed(src->var->type)) {
		movx = x64_find_OpCode(SCF_X64_MOVSX, src->var->size, dst->var->size, SCF_X64_E2G);

	} else if (src->var->size <= 2) {
		movx = x64_find_OpCode(SCF_X64_MOVZX, src->var->size, dst->var->size, SCF_X64_E2G);

	} else {
		assert(4 == src->var->size);
		xor  = x64_find_OpCode(SCF_X64_XOR, 8, 8, SCF_X64_G2E);
		inst = x64_make_inst_G2E(xor, rd, rd);
		X64_INST_ADD_CHECK(c->instructions, inst);

		movx = x64_find_OpCode(SCF_X64_MOV, 4, 4, SCF_X64_E2G);
	}

	if (src->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src, c, f, 0);
		inst = x64_make_inst_E2G(movx, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else if (0 == src->color) {
		// get the rd's low bits register
		rs   = x64_find_register_color_bytes(rd->color, src->var->size);

		mov  = x64_find_OpCode(SCF_X64_MOV, src->var->size, src->var->size, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, rs, (uint8_t*)&src->var->data, src->var->size);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_E2G(movx, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M2G(&rela, movx, rd, NULL, src->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	}

	return 0;
}

int x64_inst_float_cast(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t*   OpCode;

	scf_instruction_t*  inst;
	scf_register_x64_t* rs = NULL;
	scf_register_x64_t* rd = NULL;

	X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);

	int OpCode_type;
	if (scf_variable_float(dst->var)) {

		if (scf_variable_float(src->var)) {

			if (SCF_VAR_FLOAT == src->var->type)
				OpCode_type = SCF_X64_CVTSS2SD;
			else
				OpCode_type = SCF_X64_CVTSD2SS;

		} else {
			if (SCF_VAR_FLOAT == dst->var->type)
				OpCode_type = SCF_X64_CVTSI2SS;
			else
				OpCode_type = SCF_X64_CVTSI2SD;
		}
	} else {
		if (SCF_VAR_FLOAT == src->var->type)
			OpCode_type = SCF_X64_CVTTSS2SI;
		else
			OpCode_type = SCF_X64_CVTTSD2SI;
	}

	OpCode = x64_find_OpCode(OpCode_type, src->var->size, dst->var->size, SCF_X64_E2G);
	if (!OpCode) {
		scf_loge("OpCode_type: %d, size: %d->%d\n", OpCode_type, src->var->size, dst->var->size);
		return -EINVAL;
	}

	if (0 == src->color) {
		src->color = -1;
		src->var->global_flag = 1;

		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);

		inst   = x64_make_inst_E2G(OpCode, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else if (src->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);
		inst = x64_make_inst_E2G(OpCode, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		inst   = x64_make_inst_M2G(&rela, OpCode, rd, NULL, src->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	}

	return 0;
}

int x64_inst_jmp(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dsts || c->dsts->size != 1)
		return -EINVAL;

	scf_3ac_operand_t* dst = c->dsts->data[0];

	if (!dst->bb)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	uint32_t offset = 0;

	scf_x64_OpCode_t*  jcc  = x64_find_OpCode(OpCode_type, 4,4, SCF_X64_I);

	scf_instruction_t* inst = x64_make_inst_I(jcc, (uint8_t*)&offset, sizeof(offset));

	X64_INST_ADD_CHECK(c->instructions, inst);
	return 0;
}

