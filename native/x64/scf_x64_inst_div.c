#include"scf_x64.h"

int x64_inst_int_div(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f, int mod_flag)
{
	assert(0 != dst->color);

	int size = x64_variable_size(src->var);
	int ret;

	scf_register_x64_t* rs   = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;

	scf_x64_OpCode_t* 	mov;
	scf_x64_OpCode_t* 	cdq;
	scf_x64_OpCode_t* 	xor;
	scf_x64_OpCode_t* 	div;

	scf_register_x64_t* rl   = x64_find_register_type_id_bytes(0, SCF_X64_REG_AX, size);
	scf_register_x64_t* rh;
	if (1 == size)
		rh   = x64_find_register("ah");
	else
		rh   = x64_find_register_type_id_bytes(0, SCF_X64_REG_DX, size);

	int      src_literal = src->var->const_literal_flag;
	intptr_t src_color   = src->color;

	if (0 == src->color) {
		src->var->const_literal_flag = 0;
		src->var->tmp_flag = 1;
		src->color = -1;

		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);
	}

	if (dst->color > 0) {
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 1);

		if (rd->id != rl->id) {
			ret = x64_overflow_reg(rl, c, f);
			if (ret < 0)
				return ret;

			mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);
			inst = x64_make_inst_G2E(mov, rl, rd);
			X64_INST_ADD_CHECK(c->instructions, inst);
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

		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, mov, rl, NULL, dst->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
	}

	if (scf_variable_signed(src->var)) {
		div  = x64_find_OpCode(SCF_X64_IDIV,  size, size, SCF_X64_E);
		cdq  = x64_find_OpCode_by_type(SCF_X64_CDQ);
		inst = x64_make_inst(cdq, size << 1);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		div  = x64_find_OpCode(SCF_X64_DIV,  size, size, SCF_X64_E);
		xor  = x64_find_OpCode(SCF_X64_XOR,  size, size, SCF_X64_G2E);
		inst = x64_make_inst_G2E(xor, rh, rh);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	if (src->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src, c, f, 1);
		inst = x64_make_inst_E(div, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M(&rela, div, src->var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
	}

	scf_register_x64_t* result;
	if (mod_flag)
		result = rh;
	else
		result = rl;

	if (rd) {
		if (rd->id != result->id) {
			mov  = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_G2E);
			inst = x64_make_inst_G2E(mov, rd, result);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV, dst->var->size, dst->var->size, SCF_X64_G2E);
		inst = x64_make_inst_G2M(&rela, mov, dst->var, NULL, result);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
	}

	if (0 == src_color) {
		src->var->tmp_flag = 0;
		src->var->const_literal_flag = src_literal;

		if (src->color > 0) {
			assert(0 == scf_vector_del(rs->dag_nodes, src));
			src->loaded = 0;
		}

		src->color = 0;
	}

	return 0;
}

