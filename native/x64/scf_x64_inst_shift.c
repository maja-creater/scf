#include"scf_x64.h"

static int _shift_count(scf_dag_node_t* count, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_register_x64_t* rc   = NULL;
	scf_register_x64_t* cl   = x64_find_register("cl");

	scf_instruction_t*  inst;
	scf_x64_OpCode_t*   mov;
	scf_x64_OpCode_t*   shift;

	int ret;

	if (count->color > 0) {
		rc = x64_find_register_color(count->color);

		if (!X64_COLOR_CONFLICT(rc->color, cl->color)) {
			ret = x64_overflow_reg(cl, c, f);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			rc = x64_find_register_color_bytes(rc->color, 1);

			mov  = x64_find_OpCode(SCF_X64_MOV, 1, 1, SCF_X64_G2E);
			inst = x64_make_inst_G2E(mov, cl, rc);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else if (count->color < 0) {
		scf_rela_t* rela = NULL;

		ret = x64_overflow_reg(cl, c, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		mov  = x64_find_OpCode(SCF_X64_MOV, 1, 1, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, mov, cl, NULL, count->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, count->var, NULL);
	}

	return 0;
}

static int _x64_shift(scf_native_t* ctx, scf_3ac_code_t* c, scf_dag_node_t* dst, scf_dag_node_t* count, int OpCode_type)
{
	scf_x64_context_t* x64  = ctx->priv;
	scf_function_t*    f    = x64->f;

	if (0 == dst->color)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_register_x64_t* rd   = NULL;

	scf_instruction_t*  inst;
	scf_x64_OpCode_t*   mov;
	scf_x64_OpCode_t*   shift;

	int ret = _shift_count(count, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (dst->color > 0) {
		X64_SELECT_REG_CHECK(&rd, dst, c, f, 0);

		if (0 != count->color) {
			shift = x64_find_OpCode(OpCode_type, 1, dst->var->size, SCF_X64_G2E);
			inst  = x64_make_inst_E(shift, rd);
			X64_INST_ADD_CHECK(c->instructions, inst);
		} else {
			shift = x64_find_OpCode(OpCode_type, 1, dst->var->size, SCF_X64_I2E);
			inst = x64_make_inst_I2E(shift, rd, (uint8_t*)&count->var->data, 1);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		scf_rela_t* rela = NULL;

		if (0 != count->color) {
			shift = x64_find_OpCode(OpCode_type, 1, dst->var->size, SCF_X64_G2E);
			inst  = x64_make_inst_M(&rela, shift, dst->var, NULL);
			X64_INST_ADD_CHECK(c->instructions, inst);
			X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
		} else {
			shift = x64_find_OpCode(OpCode_type, 1, dst->var->size, SCF_X64_I2E);
			inst  = x64_make_inst_I2M(&rela, shift, dst->var, NULL, (uint8_t*)&count->var->data, 1);
			X64_INST_ADD_CHECK(c->instructions, inst);
			X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
		}
	}

	return 0;
}

int x64_shift(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dsts || c->dsts->size != 1)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_x64_context_t* x64  = ctx->priv;
	scf_function_t*    f    = x64->f;
	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];
	scf_3ac_operand_t* dst  = c->dsts->data[0];

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (!dst || !dst->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int ret = x64_inst_op2(SCF_X64_MOV, dst->dag_node, src0->dag_node, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (SCF_X64_SHR == OpCode_type
			&& scf_type_is_signed(dst->dag_node->var->type))
		OpCode_type = SCF_X64_SAR;

	return _x64_shift(ctx, c, dst->dag_node, src1->dag_node, OpCode_type);
}

int x64_shift_assign(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dsts || c->dsts->size != 1)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64   = ctx->priv;
	scf_function_t*    f     = x64->f;
	scf_3ac_operand_t* count = c->srcs->data[0];
	scf_3ac_operand_t* dst   = c->dsts->data[0];

	if (!count || !count->dag_node)
		return -EINVAL;

	if (!dst || !dst->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (SCF_X64_SHR == OpCode_type
			&& scf_type_is_signed(dst->dag_node->var->type))
		OpCode_type = SCF_X64_SAR;

	return _x64_shift(ctx, c, dst->dag_node, count->dag_node, OpCode_type);
}

