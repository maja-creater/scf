#include"scf_x64.h"

#define X64_INST_OP3_CHECK() \
	if (!c->dst || !c->dst->dag_node) \
		return -EINVAL; \
	\
	if (!c->srcs || c->srcs->size != 2) \
		return -EINVAL; \
	\
	scf_x64_context_t* x64  = ctx->priv; \
	scf_function_t*    f    = x64->f; \
	\
	scf_3ac_operand_t* dst  = c->dst; \
	scf_3ac_operand_t* src0 = c->srcs->data[0]; \
	scf_3ac_operand_t* src1 = c->srcs->data[1]; \
	\
	if (!src0 || !src0->dag_node) \
		return -EINVAL; \
	\
	if (!src1 || !src1->dag_node) \
		return -EINVAL; \
	\
	if (src0->dag_node->var->size != src1->dag_node->var->size) \
		return -EINVAL; \

static int _x64_inst_float_op2(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	if (0 == src->color) {
		src->color = -1;
		src->var->global_flag = 1;
	}

	OpCode_type = x64_float_OpCode_type(OpCode_type, src->var->type);
	if (OpCode_type < 0) {
		scf_loge("\n");
		return -1;
	}

	return x64_inst_op2(OpCode_type, dst, src, c, f);
}

static int _x64_inst_float_op3(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src0, scf_dag_node_t* src1, scf_3ac_code_t* c, scf_function_t* f)
{
	int ret  = 0;

	if (0 == src0->color) {
		src0->color = -1;
		src0->var->global_flag = 1;
	}

	if (0 == src1->color) {
		src1->color = -1;
		src1->var->global_flag = 1;
	}

	if (SCF_VAR_FLOAT == src0->var->type)
		ret = x64_inst_op2(SCF_X64_MOVSS, dst, src0, c, f);
	else
		ret = x64_inst_op2(SCF_X64_MOVSD, dst, src0, c, f);

	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	OpCode_type = x64_float_OpCode_type(OpCode_type, src0->var->type);
	if (OpCode_type < 0) {
		scf_loge("\n");
		return -1;
	}

	return x64_inst_op2(OpCode_type, dst, src1, c, f);
}

static int _x64_inst_call_stack_size(scf_3ac_code_t* c)
{
	int stack_size = 0;

	int i;
	for (i = 1; i < c->srcs->size; i++) {
		scf_3ac_operand_t*  src = c->srcs->data[i];
		scf_variable_t*     v   = src->dag_node->var;

		if (src->dag_node->rabi2)
			continue;

		int size = x64_variable_size(v);
		if (size & 0x7)
			size = (size + 7) >> 3 << 3;

		v->sp_offset  = stack_size;
		stack_size   += size;
	}
	assert(0 == (stack_size & 0x7));

	return stack_size;
}

static int _x64_inst_call_argv(scf_3ac_code_t* c, scf_function_t* f)
{
	scf_register_x64_t* rsp  = x64_find_register("rsp");

	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;

	int ret;
	int i;
	for (i = c->srcs->size - 1; i >= 1; i--) {
		scf_3ac_operand_t*  src  = c->srcs->data[i];
		scf_variable_t*     v    = src->dag_node->var;
		scf_register_x64_t* rabi = src->dag_node->rabi2;
		scf_register_x64_t* rs   = NULL;

		int size     = x64_variable_size(v);
		int is_float = scf_variable_float(v);

		if (!rabi) {
			if (!is_float)
				rabi = x64_find_register_type_id_bytes(0, SCF_X64_REG_RDI,  size);
			else
				rabi = x64_find_register_type_id_bytes(1, SCF_X64_REG_XMM0, size);

			ret = x64_overflow_reg(rabi, c, f);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}

		if (!is_float) {
			if (0 == src->dag_node->color) {
				mov  = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_I2G);
				inst = x64_make_inst_I2G(mov, rabi, (uint8_t*)&v->data, size);
				X64_INST_ADD_CHECK(c->instructions, inst);

				if (!src->dag_node->rabi2) {
					mov  = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_G2E);
					inst = x64_make_inst_G2P(mov, rsp, v->sp_offset, rabi);
					X64_INST_ADD_CHECK(c->instructions, inst);
				}
				continue;
			}

			mov = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_G2E);

		} else {
			if (0 == src->dag_node->color) {
				src->dag_node->color = -1;
				v->global_flag       =  1;
			}

			if (SCF_VAR_FLOAT == v->type)
				mov = x64_find_OpCode(SCF_X64_MOVSS, size, size, SCF_X64_G2E);
			else
				mov = x64_find_OpCode(SCF_X64_MOVSS, size, size, SCF_X64_G2E);
		}

		X64_SELECT_REG_CHECK(&rs, src->dag_node, c, f, 1);

		if (!src->dag_node->rabi2) {
			inst = x64_make_inst_G2P(mov, rsp, v->sp_offset, rs);
			X64_INST_ADD_CHECK(c->instructions, inst);
			continue;
		}

		if (!X64_COLOR_CONFLICT(rabi->color, rs->color)) {

			ret = x64_overflow_reg(rabi, c, f);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			inst = x64_make_inst_G2E(mov, rabi, rs);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	}

	return 0;
}

static int _x64_inst_call_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (c->srcs->size < 1) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_x64_context_t* x64    = ctx->priv;
	scf_function_t*    f      = x64->f;

	scf_3ac_operand_t* dst	  = c->dst;
	scf_3ac_operand_t* src0   = c->srcs->data[0];
	scf_variable_t*    var_pf = src0->dag_node->var;
	scf_function_t*    pf     = var_pf->func_ptr;

	if (SCF_FUNCTION_PTR != var_pf->type || !pf) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_x64_OpCode_t*   sub;
	scf_x64_OpCode_t*   add;
	scf_x64_OpCode_t*   call;
	scf_instruction_t*  inst;

	int stack_size = _x64_inst_call_stack_size(c);
	if (stack_size > 0) {
		sub  = x64_find_OpCode(SCF_X64_SUB,  4,4, SCF_X64_I2E);
		inst = x64_make_inst_I2E(sub, rsp, (uint8_t*)&stack_size, 4);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	int ret = _x64_inst_call_argv(c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	int32_t offset = 0;
	call = x64_find_OpCode(SCF_X64_CALL, 4,4, SCF_X64_I);
	inst = x64_make_inst_I(call, (uint8_t*)&offset, 4);
	X64_INST_ADD_CHECK(c->instructions, inst);

	if (var_pf->const_literal_flag) {
		assert(0 == src0->dag_node->color);

		scf_rela_t* rela = calloc(1, sizeof(scf_rela_t));
		if (!rela)
			return -ENOMEM;

		rela->inst_offset = 1;
		X64_RELA_ADD_CHECK(f->text_relas, rela, c, NULL, pf);
	} else {
		assert(0 != src0->dag_node->color);
		scf_loge("\n");
		return -EINVAL;
	}

	if (stack_size > 0) {
		add  = x64_find_OpCode(SCF_X64_ADD, 4, 4, SCF_X64_I2E);
		inst = x64_make_inst_I2E(add, rsp, (uint8_t*)&stack_size, 4);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}
	return 0;
}

static int _x64_inst_unary(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;
	scf_3ac_operand_t* src = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int ret = x64_inst_op2(SCF_X64_MOV, c->dst->dag_node, src->dag_node, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_instruction_t*  inst   = NULL;
	scf_register_x64_t* rd     = NULL;
	scf_variable_t*     var    = c->dst->dag_node->var;

	scf_x64_OpCode_t*   OpCode = x64_find_OpCode(OpCode_type, var->size, var->size, SCF_X64_E);
	if (!OpCode) {
		scf_loge("\n");
		return -1;
	}

	if (c->dst->dag_node->color > 0) {
		X64_SELECT_REG_CHECK(&rd, c->dst->dag_node, c, f, 0);
		inst = x64_make_inst_E(OpCode, rd);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M(&rela, OpCode, var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, var, NULL);
	}

	return 0;
}

static int _x64_inst_unary_assign(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;
	scf_3ac_operand_t* src = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_instruction_t*  inst   = NULL;
	scf_register_x64_t* rs     = NULL;
	scf_variable_t*     var    = src->dag_node->var;

	scf_x64_OpCode_t*   OpCode = x64_find_OpCode(OpCode_type, var->size, var->size, SCF_X64_E);
	if (!OpCode) {
		scf_loge("\n");
		return -1;
	}

	if (src->dag_node->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src->dag_node, c, f, 1);
		inst = x64_make_inst_E(OpCode, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else if (0 == src->dag_node->color) {
		scf_loge("\n");
		return -EINVAL;
	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M(&rela, OpCode, var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, var, NULL);
	}

	return 0;
}

static int _x64_inst_unary_post_assign(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;
	scf_3ac_operand_t* src = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_instruction_t*  inst   = NULL;
	scf_register_x64_t* rs     = NULL;
	scf_variable_t*     var    = src->dag_node->var;

	int ret = x64_inst_op2(SCF_X64_MOV, c->dst->dag_node, src->dag_node, c, f);
	if (ret < 0)
		return ret;

	return _x64_inst_unary_assign(ctx, c, OpCode_type);
}

static int _x64_inst_bit_not_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary(ctx, c, SCF_X64_NOT);
}

static int _x64_inst_neg_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary(ctx, c, SCF_X64_NEG);
}

static int _x64_inst_inc_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary_assign(ctx, c, SCF_X64_INC);
}

static int _x64_inst_inc_post_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary_post_assign(ctx, c, SCF_X64_INC);
}

static int _x64_inst_dec_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary_assign(ctx, c, SCF_X64_DEC);
}

static int _x64_inst_dec_post_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_unary_post_assign(ctx, c, SCF_X64_DEC);
}

static int _x64_inst_pointer_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return x64_inst_pointer(ctx, c);
}

static int _x64_inst_assign_array_index(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 4)
		return -EINVAL;

	scf_x64_context_t*  x64    = ctx->priv;
	scf_function_t*     f      = x64->f;

	scf_3ac_operand_t*  base   = c->srcs->data[0];
	scf_3ac_operand_t*  index  = c->srcs->data[1];
	scf_3ac_operand_t*  scale  = c->srcs->data[2];
	scf_3ac_operand_t*  src    = c->srcs->data[3];

	if (!base || !base->dag_node)
		return -EINVAL;

	if (!index || !index->dag_node)
		return -EINVAL;

	if (!scale || !scale->dag_node)
		return -EINVAL;

	if (!src || !src->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_variable_t*     vb   = base->dag_node->var;
	scf_variable_t*     vs   = src ->dag_node->var;

	scf_register_x64_t* rs   = NULL;
	x64_sib_t           sib  = {0};

	scf_x64_OpCode_t*   OpCode;
	scf_instruction_t*  inst;

	int size = x64_variable_size(vs);

	int ret  = x64_array_index_reg(&sib, base->dag_node, index->dag_node, scale->dag_node, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (vb->nb_dimentions > 1) {
		OpCode = x64_find_OpCode(SCF_X64_LEA, size, size, SCF_X64_E2G);
	} else {
		int is_float = scf_variable_float(vs);
		if (is_float) {
			OpCode_type = x64_float_OpCode_type(OpCode_type, vs->type);
			OpCode      = x64_find_OpCode(OpCode_type, size, size, SCF_X64_G2E);

			if (0 == src->dag_node->color) {
				src->dag_node->color = -1;
				vs->global_flag      =  1;
			}
		} else if (0 == src->dag_node->color) {
			OpCode = x64_find_OpCode(OpCode_type, size, size, SCF_X64_I2E);

			if (sib.index)
				inst = x64_make_inst_I2SIB(OpCode, sib.base, sib.index, sib.scale, sib.disp, (uint8_t*)&vs->data, size);
			else
				inst = x64_make_inst_I2P(OpCode, sib.base, sib.disp, (uint8_t*)&vs->data, size);
			X64_INST_ADD_CHECK(c->instructions, inst);
			return 0;
		} else {
			OpCode = x64_find_OpCode(OpCode_type, size, size, SCF_X64_G2E);
		}
	}

	if (!OpCode) {
		scf_loge("\n");
		return -EINVAL;
	}

	ret = x64_select_reg(&rs, src->dag_node, c, f, 1);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (sib.index) {
		inst = x64_make_inst_G2SIB(OpCode, sib.base, sib.index, sib.scale, sib.disp, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		inst = x64_make_inst_G2P(OpCode, sib.base, sib.disp, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}
	return 0;
}

static int _x64_inst_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 3)
		return -EINVAL;

	scf_x64_context_t*  x64    = ctx->priv;
	scf_function_t*     f      = x64->f;

	scf_3ac_operand_t*  base   = c->srcs->data[0];
	scf_3ac_operand_t*  index  = c->srcs->data[1];
	scf_3ac_operand_t*  scale  = c->srcs->data[2];

	if (!base || !base->dag_node)
		return -EINVAL;

	if (!index || !index->dag_node)
		return -EINVAL;

	if (!scale || !scale->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_variable_t*     vd  = c->dst->dag_node->var;
	scf_variable_t*     vb  = base  ->dag_node->var;
	scf_variable_t*     vi  = index ->dag_node->var;
	scf_variable_t*     vs  = scale ->dag_node->var;

	scf_register_x64_t* rd  = NULL;
	x64_sib_t           sib = {0};

	scf_x64_OpCode_t*   OpCode;
	scf_instruction_t*  inst;

	int ret = x64_select_reg(&rd, c->dst->dag_node, c, f, 0);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	ret = x64_array_index_reg(&sib, base->dag_node, index->dag_node, scale->dag_node, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (vb->nb_dimentions > 1) {
		OpCode = x64_find_OpCode(SCF_X64_LEA, rd->bytes, rd->bytes, SCF_X64_E2G);

	} else {
		int is_float = scf_variable_float(vd);
		if (is_float) {
			if (SCF_VAR_FLOAT == vd->type)
				OpCode = x64_find_OpCode(SCF_X64_MOVSS, rd->bytes, rd->bytes, SCF_X64_E2G);
			else if (SCF_VAR_DOUBLE == vd->type)
				OpCode = x64_find_OpCode(SCF_X64_MOVSD, rd->bytes, rd->bytes, SCF_X64_E2G);
		} else
			OpCode = x64_find_OpCode(SCF_X64_MOV,   rd->bytes, rd->bytes, SCF_X64_E2G);
	}

	if (!OpCode) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (sib.index) {
		inst = x64_make_inst_SIB2G(OpCode, rd, sib.base, sib.index, sib.scale, sib.disp);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		inst = x64_make_inst_P2G(OpCode, rd, sib.base, sib.disp);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}
	return 0;
}

static int _x64_inst_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return x64_inst_dereference(ctx, c);
}

static int _x64_inst_address_of_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->srcs || c->srcs->size != 1) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_x64_context_t*  x64  = ctx->priv;
	scf_function_t*     f    = x64->f;

	scf_3ac_operand_t*  dst  = c->dst;
	scf_3ac_operand_t*  src  = c->srcs->data[0];
	scf_register_x64_t* rd   = NULL;
	scf_rela_t*         rela = NULL;

	scf_x64_OpCode_t*   lea;
	scf_instruction_t*  inst;

	if (!src || !src->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}
	assert(dst->dag_node->var->nb_pointers > 0);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int ret = x64_select_reg(&rd, dst->dag_node, c, f, 0);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	assert(dst->dag_node->color > 0);

	lea  = x64_find_OpCode(SCF_X64_LEA, 8,8, SCF_X64_E2G);
	inst = x64_make_inst_M2G(&rela, lea, rd, NULL, src->dag_node->var);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->dag_node->var, NULL);
	return 0;
}

static int _div_mod(scf_native_t* ctx, scf_3ac_code_t* c, int mod_flag)
{
	X64_INST_OP3_CHECK()

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (scf_variable_float(src0->dag_node->var)) {

		assert(scf_variable_float(src1->dag_node->var));
		assert(scf_variable_float(dst->dag_node->var));

		return _x64_inst_float_op3(SCF_X64_DIV, dst->dag_node, src0->dag_node, src1->dag_node, c, f);
	}

	int ret = x64_inst_op2(SCF_X64_MOV, dst->dag_node, src0->dag_node, c, f);
	if (ret < 0)
		return ret;

	return x64_inst_int_div(dst->dag_node, src1->dag_node, c, f, mod_flag);
}

static int _x64_inst_div_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _div_mod(ctx, c, 0);
}

static int _x64_inst_mod_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _div_mod(ctx, c, 1);
}

static int _x64_inst_mul_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	X64_INST_OP3_CHECK()

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (scf_variable_float(src0->dag_node->var)) {

		assert(scf_variable_float(src1->dag_node->var));
		assert(scf_variable_float(dst->dag_node->var));

		return _x64_inst_float_op3(SCF_X64_MUL, dst->dag_node, src0->dag_node, src1->dag_node, c, f);
	}

	int ret = x64_inst_op2(SCF_X64_MOV, dst->dag_node, src0->dag_node, c, f);
	if (ret < 0)
		return ret;
	return x64_inst_int_mul(dst->dag_node, src1->dag_node, c, f);
}

static int _x64_inst_op3(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	X64_INST_OP3_CHECK()

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (scf_variable_float(src0->dag_node->var)) {

		assert(scf_variable_float(src1->dag_node->var));
		assert(scf_variable_float(dst->dag_node->var));

		return _x64_inst_float_op3(OpCode_type, dst->dag_node, src0->dag_node, src1->dag_node, c, f);
	}

	int ret = x64_inst_op2(SCF_X64_MOV, dst->dag_node, src0->dag_node, c, f);
	if (ret < 0)
		return ret;
	return x64_inst_op2(OpCode_type, dst->dag_node, src1->dag_node, c, f);
}

#define X64_INST_OP3(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return _x64_inst_op3(ctx, c, SCF_X64_##op); \
}
X64_INST_OP3(add,     ADD)
X64_INST_OP3(sub,     SUB)
X64_INST_OP3(bit_and, AND)
X64_INST_OP3(bit_or,  OR)


static int _x64_inst_teq_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return x64_inst_teq(ctx, c);
}

static int _x64_inst_logic_not_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	int ret = x64_inst_teq(ctx, c);
	if (ret < 0)
		return ret;

	return x64_inst_set(ctx, c, SCF_X64_SETZ);
}

static int _x64_inst_cmp_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return x64_inst_cmp(ctx, c);
}

#define X64_INST_SET(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_inst_set(ctx, c, SCF_X64_##op); \
}
X64_INST_SET(setz,  SETZ)
X64_INST_SET(setnz, SETNZ)


#define X64_INST_CMP_SET(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_inst_cmp_set(ctx, c, SCF_X64_##op); \
}
X64_INST_CMP_SET(eq, SETZ)
X64_INST_CMP_SET(ne, SETNZ)
X64_INST_CMP_SET(gt, SETG)
X64_INST_CMP_SET(ge, SETGE)
X64_INST_CMP_SET(lt, SETL)
X64_INST_CMP_SET(le, SETLE)

static int _x64_inst_cast_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;
	scf_3ac_operand_t* src = c->srcs->data[0];
	scf_3ac_operand_t* dst = c->dst;

	if (!src || !src->dag_node)
		return -EINVAL;

	if (0 == dst->dag_node->color)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_x64_OpCode_t*   lea;
	scf_instruction_t*  inst;

	scf_register_x64_t* rd   = NULL;
	scf_rela_t*         rela = NULL;

	scf_variable_t*     vd   = dst->dag_node->var;
	scf_variable_t*     vs   = src->dag_node->var;

	int src_size = x64_variable_size(vs);
	int dst_size = x64_variable_size(vd);

	if (scf_variable_float(vs) || scf_variable_float(vd))
		return x64_inst_float_cast(dst->dag_node, src->dag_node, c, f);

	if (src_size < dst_size)
		return x64_inst_movx(dst->dag_node, src->dag_node, c, f);

	scf_logw("src_size: %d, dst_size: %d\n", src_size, dst_size);
	return x64_inst_op2(SCF_X64_MOV, dst->dag_node, src->dag_node, c, f);
}

static int _x64_inst_mul_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t* x64  = ctx->priv;
	scf_function_t*    f    = x64->f;

	scf_3ac_operand_t* dst  = c->dst;
	scf_3ac_operand_t* src  = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	if (src->dag_node->var->size != dst->dag_node->var->size)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (scf_variable_float(src->dag_node->var)) {

		assert(scf_variable_float(dst->dag_node->var));

		return _x64_inst_float_op2(SCF_X64_MUL, dst->dag_node, src->dag_node, c, f);
	}

	return x64_inst_int_mul(dst->dag_node, src->dag_node, c, f);
}

static int _div_mod_assign(scf_native_t* ctx, scf_3ac_code_t* c, int mod_flag)
{
	if (!c->dst || !c->dst->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->srcs || c->srcs->size != 1) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_x64_context_t*  x64  = ctx->priv;
	scf_function_t*     f    = x64->f;

	scf_3ac_operand_t* dst  = c->dst;
	scf_3ac_operand_t* src  = c->srcs->data[0];

	if (!src || !src->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (src->dag_node->var->size != dst->dag_node->var->size) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (scf_variable_float(src->dag_node->var)) {
		assert(scf_variable_float(dst->dag_node->var));

		return _x64_inst_float_op2(SCF_X64_DIV, dst->dag_node, src->dag_node, c, f);
	}

	return x64_inst_int_div(dst->dag_node, src->dag_node, c, f, mod_flag);
}

static int _x64_inst_div_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _div_mod_assign(ctx, c, 0);
}

static int _x64_inst_mod_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _div_mod_assign(ctx, c, 1);
}

static int _x64_inst_return_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_x64_context_t*  x64  = ctx->priv;
	scf_function_t*     f    = x64->f;
	scf_3ac_operand_t*  src  = c->srcs->data[0];
	scf_instruction_t*  inst = NULL;
	scf_rela_t*         rela = NULL;

	scf_register_x64_t*	rd   = NULL;
	scf_register_x64_t*	rs   = NULL;
	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_register_x64_t* rbp  = x64_find_register("rbp");

	scf_x64_OpCode_t*   pop;
	scf_x64_OpCode_t*   mov;
	scf_x64_OpCode_t*   ret;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	if (!src || !src->dag_node) {
		rd = x64_find_register_type_id_bytes(0, 0, 8);
		goto end;
	}

	scf_variable_t* v = src->dag_node->var;

	int size     = x64_variable_size(v);
	int is_float = scf_variable_float(v);

	if (8 == size)
		rd = x64_find_register_type_id_bytes(is_float, 0, 8);
	else
		rd = x64_find_register_type_id_bytes(is_float, 0, 4);

	if (is_float) {
		if (0 == src->dag_node->color) {
			src->dag_node->color = -1;
			v->global_flag       =  1;
		}

		if (SCF_VAR_FLOAT == v->type)
			mov = x64_find_OpCode(SCF_X64_MOVSS, size, rd->bytes, SCF_X64_E2G);
		else
			mov = x64_find_OpCode(SCF_X64_MOVSD, size, rd->bytes, SCF_X64_E2G);
		goto mov;
	}

	if (0 == src->dag_node->color) {
		if (rd->bytes > size)
			scf_variable_extend_bytes(v, rd->bytes);

		mov = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, rd, (uint8_t*)&v->data, rd->bytes);
		X64_INST_ADD_CHECK(c->instructions, inst);
		goto end;
	}

	if (rd->bytes > size) {
		if (scf_variable_signed(v))
			mov = x64_find_OpCode(SCF_X64_MOVSX, size, rd->bytes, SCF_X64_E2G);
		else
			mov = x64_find_OpCode(SCF_X64_MOVZX, size, rd->bytes, SCF_X64_E2G);
	} else {
		mov = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_E2G);
	}

mov:
	if (src->dag_node->color > 0) {
		X64_SELECT_REG_CHECK(&rs, src->dag_node, c, f, 1);

		if (!X64_COLOR_CONFLICT(rd->color, rs->color)) {
			inst = x64_make_inst_E2G(mov, rd, rs);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		inst = x64_make_inst_M2G(&rela, mov, rd, NULL, v);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, v, NULL);
	}

end:
	return 0;
}

static int _x64_inst_nop_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return 0;
}

static int _x64_inst_end_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_register_x64_t* rbp  = x64_find_register("rbp");

	scf_x64_OpCode_t*   pop  = x64_find_OpCode(SCF_X64_POP, 8, 8, SCF_X64_G);
	scf_x64_OpCode_t*   mov  = x64_find_OpCode(SCF_X64_MOV, 8, 8, SCF_X64_G2E);
	scf_x64_OpCode_t*   ret  = x64_find_OpCode(SCF_X64_RET, 8, 8, SCF_X64_G);
	scf_instruction_t*  inst = NULL;

	inst = x64_make_inst_G2E(mov, rsp, rbp);
	X64_INST_ADD_CHECK(c->instructions, inst);

	inst = x64_make_inst_G(pop, rbp);
	X64_INST_ADD_CHECK(c->instructions, inst);

	inst = x64_make_inst(ret, 8);
	X64_INST_ADD_CHECK(c->instructions, inst);
	return 0;
}

#define X64_INST_JMP(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_inst_jmp(ctx, c, SCF_X64_##op); \
}

X64_INST_JMP(goto, JMP)
X64_INST_JMP(jz,   JZ)
X64_INST_JMP(jnz,  JNZ)
X64_INST_JMP(jgt,  JG)
X64_INST_JMP(jge,  JGE)
X64_INST_JMP(jlt,  JL)
X64_INST_JMP(jle,  JLE)

static int _x64_inst_load_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t*  x64 = ctx->priv;
	scf_function_t*     f   = x64->f;
	scf_dag_node_t*     dn  = c->dst->dag_node;

	if (dn->color < 0)
		return 0;
	assert(dn->color > 0);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_register_x64_t* r    = x64_find_register_color(dn->color);

	return x64_load_reg(r, dn, c, f);
}

static int _x64_inst_reload_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t*  x64 = ctx->priv;
	scf_function_t*     f   = x64->f;
	scf_dag_node_t*     dn  = c->dst->dag_node;

	if (dn->color < 0)
		return 0;
	assert(dn->color > 0);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_register_x64_t* r = x64_find_register_color(dn->color);

	dn->loaded = 0;
	return x64_load_reg(r, dn, c, f);
}

static int _x64_inst_save_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_3ac_operand_t*  src = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	scf_x64_context_t*  x64 = ctx->priv;
	scf_function_t*     f   = x64->f;
	scf_dag_node_t*     dn  = src->dag_node;

	if (dn->color < 0)
		return 0;
	assert(dn->color > 0);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	return x64_save_var(dn, c, f);
}

#define X64_INST_BINARY_ASSIGN(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_binary_assign(ctx, c, SCF_X64_##op); \
} \
static int _x64_inst_##name##_pointer_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_binary_assign_pointer(ctx, c, SCF_X64_##op); \
} \
static int _x64_inst_##name##_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	return _x64_inst_assign_array_index(ctx, c, SCF_X64_##op);\
}\
static int _x64_inst_##name##_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	return x64_binary_assign_dereference(ctx, c, SCF_X64_##op);\
}

X64_INST_BINARY_ASSIGN(assign,     MOV)
X64_INST_BINARY_ASSIGN(add_assign, ADD)
X64_INST_BINARY_ASSIGN(sub_assign, SUB)
X64_INST_BINARY_ASSIGN(and_assign, AND)
X64_INST_BINARY_ASSIGN(or_assign,  OR)

#define X64_INST_SHIFT(name, op) \
static int _x64_inst_##name##_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_shift(ctx, c, SCF_X64_##op); \
} \
static int _x64_inst_##name##_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_shift_assign(ctx, c, SCF_X64_##op); \
}
X64_INST_SHIFT(shl, SHL)
X64_INST_SHIFT(shr, SHR)

#define X64_INST_UNARY_ASSIGN(name, op) \
static int _x64_inst_##name##_pointer_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	return x64_unary_assign_pointer(ctx, c, SCF_X64_##op); \
} \
static int _x64_inst_##name##_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	return x64_unary_assign_array_index(ctx, c, SCF_X64_##op);\
}\
static int _x64_inst_##name##_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	return x64_unary_assign_dereference(ctx, c, SCF_X64_##op);\
}
X64_INST_UNARY_ASSIGN(inc, INC)
X64_INST_UNARY_ASSIGN(dec, DEC)

#define X64_INST_UNARY_POST_ASSIGN(name, op) \
static int _x64_inst_##name##_pointer_handler(scf_native_t* ctx, scf_3ac_code_t* c) \
{ \
	int ret = x64_inst_pointer(ctx, c); \
	if (ret < 0) \
		return ret; \
	return x64_unary_assign_pointer(ctx, c, SCF_X64_##op); \
} \
static int _x64_inst_##name##_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	int ret = _x64_inst_array_index_handler(ctx, c); \
	if (ret < 0) \
		return ret; \
	return x64_unary_assign_array_index(ctx, c, SCF_X64_##op);\
}\
static int _x64_inst_##name##_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c)\
{\
	int ret = x64_inst_dereference(ctx, c); \
	if (ret < 0) \
		return ret; \
	return x64_unary_assign_dereference(ctx, c, SCF_X64_##op);\
}
X64_INST_UNARY_POST_ASSIGN(inc_post, INC)
X64_INST_UNARY_POST_ASSIGN(dec_post, DEC)

static x64_inst_handler_t x64_inst_handlers[] = {

	{SCF_OP_CALL,			_x64_inst_call_handler},
	{SCF_OP_ARRAY_INDEX, 	_x64_inst_array_index_handler},
	{SCF_OP_POINTER,        _x64_inst_pointer_handler},

	{SCF_OP_TYPE_CAST,      _x64_inst_cast_handler},
	{SCF_OP_LOGIC_NOT, 		_x64_inst_logic_not_handler},
	{SCF_OP_BIT_NOT,        _x64_inst_bit_not_handler},
	{SCF_OP_NEG, 			_x64_inst_neg_handler},

	{SCF_OP_INC,            _x64_inst_inc_handler},
	{SCF_OP_DEC,            _x64_inst_dec_handler},

	{SCF_OP_INC_POST,       _x64_inst_inc_post_handler},
	{SCF_OP_DEC_POST,       _x64_inst_dec_post_handler},

	{SCF_OP_DEREFERENCE, 	_x64_inst_dereference_handler},
	{SCF_OP_ADDRESS_OF, 	_x64_inst_address_of_handler},

	{SCF_OP_MUL, 			_x64_inst_mul_handler},
	{SCF_OP_DIV, 			_x64_inst_div_handler},
	{SCF_OP_MOD,            _x64_inst_mod_handler},

	{SCF_OP_ADD, 			_x64_inst_add_handler},
	{SCF_OP_SUB, 			_x64_inst_sub_handler},

	{SCF_OP_SHL,            _x64_inst_shl_handler},
	{SCF_OP_SHR,            _x64_inst_shr_handler},

	{SCF_OP_BIT_AND,        _x64_inst_bit_and_handler},
	{SCF_OP_BIT_OR,         _x64_inst_bit_or_handler},

	{SCF_OP_3AC_TEQ,        _x64_inst_teq_handler},
	{SCF_OP_3AC_CMP,        _x64_inst_cmp_handler},

	{SCF_OP_3AC_SETZ,       _x64_inst_setz_handler},
	{SCF_OP_3AC_SETNZ,      _x64_inst_setnz_handler},

	{SCF_OP_EQ, 			_x64_inst_eq_handler},
	{SCF_OP_NE, 			_x64_inst_ne_handler},
	{SCF_OP_GT, 			_x64_inst_gt_handler},
	{SCF_OP_GE,             _x64_inst_ge_handler},
	{SCF_OP_LT,             _x64_inst_lt_handler},
	{SCF_OP_LE,             _x64_inst_le_handler},

	{SCF_OP_ASSIGN, 		_x64_inst_assign_handler},

	{SCF_OP_ADD_ASSIGN,     _x64_inst_add_assign_handler},
	{SCF_OP_SUB_ASSIGN,     _x64_inst_sub_assign_handler},

	{SCF_OP_MUL_ASSIGN,     _x64_inst_mul_assign_handler},
	{SCF_OP_DIV_ASSIGN,     _x64_inst_div_assign_handler},
	{SCF_OP_MOD_ASSIGN,     _x64_inst_mod_assign_handler},

	{SCF_OP_SHL_ASSIGN,     _x64_inst_shl_assign_handler},
	{SCF_OP_SHR_ASSIGN,     _x64_inst_shr_assign_handler},

	{SCF_OP_AND_ASSIGN,     _x64_inst_and_assign_handler},
	{SCF_OP_OR_ASSIGN,      _x64_inst_or_assign_handler},

	{SCF_OP_RETURN, 		_x64_inst_return_handler},
	{SCF_OP_GOTO, 			_x64_inst_goto_handler},

	{SCF_OP_3AC_JZ, 		_x64_inst_jz_handler},
	{SCF_OP_3AC_JNZ, 		_x64_inst_jnz_handler},
	{SCF_OP_3AC_JGT, 		_x64_inst_jgt_handler},
	{SCF_OP_3AC_JGE, 		_x64_inst_jge_handler},
	{SCF_OP_3AC_JLT, 		_x64_inst_jlt_handler},
	{SCF_OP_3AC_JLE, 		_x64_inst_jle_handler},

	{SCF_OP_3AC_NOP, 		_x64_inst_nop_handler},
	{SCF_OP_3AC_END, 		_x64_inst_end_handler},

	{SCF_OP_3AC_SAVE,       _x64_inst_save_handler},
	{SCF_OP_3AC_LOAD,       _x64_inst_load_handler},

	{SCF_OP_3AC_RESAVE,     _x64_inst_save_handler},
	{SCF_OP_3AC_RELOAD,     _x64_inst_reload_handler},

	{SCF_OP_3AC_ASSIGN_DEREFERENCE,     _x64_inst_assign_dereference_handler},
	{SCF_OP_3AC_ASSIGN_ARRAY_INDEX,     _x64_inst_assign_array_index_handler},
	{SCF_OP_3AC_ASSIGN_POINTER,         _x64_inst_assign_pointer_handler},

	{SCF_OP_3AC_ADD_ASSIGN_DEREFERENCE, _x64_inst_add_assign_dereference_handler},
	{SCF_OP_3AC_ADD_ASSIGN_ARRAY_INDEX, _x64_inst_add_assign_array_index_handler},
	{SCF_OP_3AC_ADD_ASSIGN_POINTER,     _x64_inst_add_assign_pointer_handler},

	{SCF_OP_3AC_SUB_ASSIGN_DEREFERENCE, _x64_inst_sub_assign_dereference_handler},
	{SCF_OP_3AC_SUB_ASSIGN_ARRAY_INDEX, _x64_inst_sub_assign_array_index_handler},
	{SCF_OP_3AC_SUB_ASSIGN_POINTER,     _x64_inst_sub_assign_pointer_handler},

	{SCF_OP_3AC_AND_ASSIGN_DEREFERENCE, _x64_inst_and_assign_dereference_handler},
	{SCF_OP_3AC_AND_ASSIGN_ARRAY_INDEX, _x64_inst_and_assign_array_index_handler},
	{SCF_OP_3AC_AND_ASSIGN_POINTER,     _x64_inst_and_assign_pointer_handler},

	{SCF_OP_3AC_OR_ASSIGN_DEREFERENCE,  _x64_inst_or_assign_dereference_handler},
	{SCF_OP_3AC_OR_ASSIGN_ARRAY_INDEX,  _x64_inst_or_assign_array_index_handler},
	{SCF_OP_3AC_OR_ASSIGN_POINTER,      _x64_inst_or_assign_pointer_handler},

	{SCF_OP_3AC_INC_DEREFERENCE,        _x64_inst_inc_dereference_handler},
	{SCF_OP_3AC_INC_ARRAY_INDEX,        _x64_inst_inc_array_index_handler},
	{SCF_OP_3AC_INC_POINTER,            _x64_inst_inc_pointer_handler},

	{SCF_OP_3AC_INC_POST_DEREFERENCE,   _x64_inst_inc_post_dereference_handler},
	{SCF_OP_3AC_INC_POST_ARRAY_INDEX,   _x64_inst_inc_post_array_index_handler},
	{SCF_OP_3AC_INC_POST_POINTER,       _x64_inst_inc_post_pointer_handler},

	{SCF_OP_3AC_DEC_DEREFERENCE,        _x64_inst_dec_dereference_handler},
	{SCF_OP_3AC_DEC_ARRAY_INDEX,        _x64_inst_dec_array_index_handler},
	{SCF_OP_3AC_DEC_POINTER,            _x64_inst_dec_pointer_handler},

	{SCF_OP_3AC_DEC_POST_DEREFERENCE,   _x64_inst_dec_post_dereference_handler},
	{SCF_OP_3AC_DEC_POST_ARRAY_INDEX,   _x64_inst_dec_post_array_index_handler},
	{SCF_OP_3AC_DEC_POST_POINTER,       _x64_inst_dec_post_pointer_handler},
};

x64_inst_handler_t* scf_x64_find_inst_handler(const int op_type)
{
	int i;
	for (i = 0; i < sizeof(x64_inst_handlers) / sizeof(x64_inst_handlers[0]); i++) {

		x64_inst_handler_t* h = &(x64_inst_handlers[i]);

		if (op_type == h->type)
			return h;
	}
	return NULL;
}
