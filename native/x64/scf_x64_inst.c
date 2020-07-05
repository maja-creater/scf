#include"scf_x64.h"

static int _x64_inst_op2_imm(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t*   OpCode;
	scf_instruction_t*  inst;
	scf_register_x64_t* rd;
	scf_register_x64_t* rs;

	assert(scf_variable_const(src->var));

	int src_size = src->var->size > dst->var->size ? dst->var->size : src->var->size;

	if (dst->color > 0) {
		rd     = x64_find_register_color(dst->color);
		OpCode = x64_find_OpCode(OpCode_type, src_size, dst->var->size, SCF_X64_I2G);

		if (!OpCode) {
			OpCode = x64_find_OpCode(OpCode_type, src_size, dst->var->size, SCF_X64_I2E);
			if (!OpCode)
				return -EINVAL;

			inst = x64_make_inst_I2E(OpCode, rd, (uint8_t*)&src->var->data, src_size);
		} else {
			inst = x64_make_inst_I2G(OpCode, rd, (uint8_t*)&src->var->data, src_size);
		}
		X64_INST_ADD_CHECK(c->instructions, inst);
		return 0;
	}

	OpCode = x64_find_OpCode(OpCode_type, src_size, dst->var->size, SCF_X64_I2E);
	if (!OpCode)
		return -EINVAL;

	scf_rela_t* rela = NULL;

	inst = x64_make_inst_I2M(&rela, OpCode, dst->var, NULL, (uint8_t*)&src->var->data, src_size);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);

	return 0;
}

static int _x64_inst_select_reg(scf_register_x64_t** preg, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f, int load_flag)
{
	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;
	scf_register_x64_t* r;
	int ret;

	if (0 == dn->color)
		return -EINVAL;

	if (dn->color > 0) {
		*preg = x64_find_register_color(dn->color);
		return 0;
	}

	r   = x64_select_overflowed_reg(c, dn->var->size);

	ret = x64_overflow_reg(r, c, f);
	if (ret < 0) {
		scf_loge("overflow reg failed\n");
		return ret;
	}
	assert(0 == r->dag_nodes->size);

	scf_loge("dn->var->size: %d, r->bytes: %d\n", dn->var->size, r->bytes);

	ret = scf_vector_add(r->dag_nodes, dn);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	dn->color = r->color;

	if (load_flag) {
		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV, dn->var->size, dn->var->size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, mov, r, NULL, dn->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dn->var, NULL);
	}

	*preg = r;
	return 0;
}

static int _x64_inst_op_movx(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t*   movx;
	scf_x64_OpCode_t*   mov;
	scf_x64_OpCode_t*   push;
	scf_x64_OpCode_t*   pop;
	scf_x64_OpCode_t*   xor;

	scf_instruction_t*  inst;
	scf_register_x64_t* rs;
	scf_register_x64_t* rd = NULL;

	int ret = _x64_inst_select_reg(&rd, dst, c, f, 0);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	assert(dst->color > 0);

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
		rs   = x64_find_register_color(src->color);
		inst = x64_make_inst_E2G(movx, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else if (0 == src->color) {
		// get the rd's low bits register
		rs   = x64_find_register_id_bytes(rd->id, src->var->size);

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

static int _x64_inst_op2(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f)
{
	assert(0 != dst->color);

	scf_x64_OpCode_t*   OpCode = NULL;
	scf_register_x64_t* rs     = NULL;
	scf_register_x64_t* rd     = NULL;
	scf_instruction_t*  inst   = NULL;
	scf_rela_t*         rela   = NULL;

	if (0 == src->color)
		return _x64_inst_op2_imm(OpCode_type, dst, src, c, f);

	int src_size = src->var->size > dst->var->size ? dst->var->size : src->var->size;

	int ret = _x64_inst_select_reg(&rd, dst, c, f, 0);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	assert(dst->color > 0);

	if (src->color > 0) {
		OpCode = x64_find_OpCode(OpCode_type, src_size, dst->var->size, SCF_X64_G2E);
		if (!OpCode) {
			scf_loge("OpCode_type: %d, size: %d, size: %d\n", OpCode_type, src->var->size, dst->var->size);
			return -EINVAL;
		}

		rs = x64_find_register_color(src->color);
		inst = x64_make_inst_G2E(OpCode, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		OpCode = x64_find_OpCode(OpCode_type, src_size, dst->var->size, SCF_X64_E2G);
		if (!OpCode) {
			scf_loge("OpCode_type: %d, size: %d, size: %d\n", OpCode_type, src->var->size, dst->var->size);
			return -EINVAL;
		}

		inst = x64_make_inst_M2G(&rela, OpCode, rd, NULL, src->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src->var, NULL);
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

	scf_x64_OpCode_t*  call = x64_find_OpCode(SCF_X64_CALL, 4,4, SCF_X64_I);
	scf_x64_OpCode_t*  push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t*  pop  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);

	int i;
	for (i = 0; i < X64_ABI_NB && i < pf->argv->size; i++) {
		scf_register_x64_t* r    = x64_find_register_id_bytes(x64_abi_regs[i], 8);
		scf_instruction_t*  save = x64_make_inst_G(push, r);

		X64_INST_ADD_CHECK(c->instructions, save);
	}

	for (i = 1; i < c->srcs->size; i++) {
		scf_3ac_operand_t*  src  = c->srcs->data[i];
		scf_variable_t*     arg  = src->dag_node->var;
		scf_register_x64_t* rabi = x64_find_register_id_bytes(x64_abi_regs[i - 1], arg->size);
		scf_x64_OpCode_t*   mov;
		scf_instruction_t*  inst;

		if (src->dag_node->color > 0) {
			scf_register_x64_t* rs = x64_find_register_color(src->dag_node->color);

			if (rs->id != rabi->id) {
				mov  = x64_find_OpCode(SCF_X64_MOV, arg->size, arg->size, SCF_X64_G2E);
				inst = x64_make_inst_G2E(mov, rabi, rs);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}
		} else if (0 == src->dag_node->color) {
			mov  = x64_find_OpCode(SCF_X64_MOV, arg->size, arg->size, SCF_X64_I2G);
			inst = x64_make_inst_I2G(mov, rabi, (uint8_t*)&arg->data, arg->size);
			X64_INST_ADD_CHECK(c->instructions, inst);

		} else {
			scf_rela_t* rela = NULL;

			mov  = x64_find_OpCode(SCF_X64_MOV, arg->size, arg->size, SCF_X64_E2G);
			inst = x64_make_inst_M2G(&rela, mov, rabi, NULL, arg);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	}

	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	if (!inst)
		return -ENOMEM;

	inst->OpCode 			= (scf_OpCode_t*)call;
	inst->code[inst->len++] = call->OpCodes[0];
	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
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

	i = pf->argv->size > X64_ABI_NB ? X64_ABI_NB - 1 : pf->argv->size - 1;
	for (; i >= 0; i--) {
		scf_register_x64_t* r    = x64_find_register_id_bytes(x64_abi_regs[i], 8);
		scf_instruction_t*  load = x64_make_inst_G(pop, r);

		X64_INST_ADD_CHECK(c->instructions, load);
	}

	return 0;
}

static int _x64_inst_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}

static int _x64_inst_logic_not_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}
static int _x64_inst_neg_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}

static int _x64_inst_positive_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}
static int _x64_inst_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}

static int _x64_inst_address_of_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	scf_loge("\n");
	return -1;
}

static int _x64_inst_div_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->srcs || c->srcs->size != 2) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_3ac_operand_t* dst  = c->dst;
	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];

	if (!src0 || !src0->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!src1 || !src1->dag_node) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (src0->dag_node->var->size != src1->dag_node->var->size) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (src0->dag_node->var->size < 4) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_x64_context_t*  x64  = ctx->priv;
	scf_function_t*     f    = x64->f;

	scf_register_x64_t* rs0  = NULL;
	scf_register_x64_t* rs1  = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;

	int size = src0->dag_node->var->size;

	scf_x64_OpCode_t*	push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* 	pop  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);
	scf_register_x64_t* ax   = x64_find_register_id_bytes(SCF_X64_REG_AX, size);
	scf_register_x64_t* dx   = x64_find_register_id_bytes(SCF_X64_REG_DX, size);
	scf_x64_OpCode_t* 	mov  = NULL;
	scf_x64_OpCode_t* 	cdq  = NULL;
	scf_x64_OpCode_t* 	xor  = NULL;
	scf_x64_OpCode_t* 	div  = NULL;
	scf_x64_OpCode_t* 	cx   = NULL;

	if (src0->dag_node->color > 0) {
		if (X64_COLOR_CONFLICT(src0->dag_node->color, ax->color)
				|| X64_COLOR_CONFLICT(src0->dag_node->color, dx->color)) {
			scf_loge("register select BUG\n");
			return -1;
		}
	}

	if (src1->dag_node->color > 0) {
		if (X64_COLOR_CONFLICT(src1->dag_node->color, ax->color)
				|| X64_COLOR_CONFLICT(src1->dag_node->color, dx->color)) {
			scf_loge("register select BUG\n");
			return -1;
		}
	}

	if (dst->dag_node->color > 0) {
		rd = x64_find_register_color(dst->dag_node->color);

		if (rd->id != ax->id) {
			inst = x64_make_inst_G(push, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}

		if (rd->id != dx->id) {
			inst = x64_make_inst_G(push, dx);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		inst = x64_make_inst_G(push, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_G(push, dx);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	if (src0->dag_node->color > 0) {
		rs0 = x64_find_register_color(src0->dag_node->color);

		if (rs0->id != ax->id) {
			mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);
			inst = x64_make_inst_G2E(mov, ax, rs0);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else if (0 == src0->dag_node->color) {
		mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, ax, (uint8_t*)&src0->dag_node->var->data, size);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, mov, ax, NULL, src0->dag_node->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src0->dag_node->var, NULL);
	}

	if (scf_type_is_signed(src0->dag_node->var->type)) {
		cdq  = x64_find_OpCode_by_type(SCF_X64_CDQ);
		inst = x64_make_inst_G(cdq, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		xor  = x64_find_OpCode(SCF_X64_XOR,  size, size, SCF_X64_G2E);
		inst = x64_make_inst_G2E(cdq, dx, dx);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	if (scf_type_is_signed(src0->dag_node->var->type)
			|| scf_type_is_signed(src1->dag_node->var->type))
		div = x64_find_OpCode(SCF_X64_IDIV,  size, size, SCF_X64_E);
	else
		div = x64_find_OpCode(SCF_X64_DIV,  size, size, SCF_X64_E);

	if (src1->dag_node->color > 0) {
		rs1 = x64_find_register_color(src1->dag_node->color);
		inst = x64_make_inst_E(div, rs1);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else if (0 == src1->dag_node->color) {
		return -EINVAL;
	} else {
		scf_rela_t* rela = NULL;

		inst = x64_make_inst_M(&rela, div, src1->dag_node->var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->dag_node->var, NULL);
	}

	if (rd) {
		if (rd->id != ax->id) {
			mov  = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_G2E);
			inst = x64_make_inst_G2E(mov, rd, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);

			inst = x64_make_inst_G(pop, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}

		if (rd->id != dx->id) {
			inst = x64_make_inst_G(pop, dx);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV, dst->dag_node->var->size, dst->dag_node->var->size, SCF_X64_G2E);
		inst = x64_make_inst_G2M(&rela, mov, dst->dag_node->var, NULL, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->dag_node->var, NULL);

		inst = x64_make_inst_G(pop, dx);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_G(pop, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	return 0;
}

static int _x64_inst_mul_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_3ac_operand_t* dst  = c->dst;
	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (src0->dag_node->var->size != src1->dag_node->var->size)
		return -EINVAL;

	if (src0->dag_node->var->size < 4)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_x64_context_t*  x64  = ctx->priv;
	scf_function_t*     f    = x64->f;

	scf_register_x64_t* rs0  = NULL;
	scf_register_x64_t* rs1  = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;

	int size = src0->dag_node->var->size;

	scf_x64_OpCode_t*	push_OpCode = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* 	pop_OpCode  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);
	scf_x64_OpCode_t* 	mov_OpCode  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);
	scf_register_x64_t* ax          = x64_find_register_id_bytes(SCF_X64_REG_AX, size);
	scf_register_x64_t* dx          = x64_find_register_id_bytes(SCF_X64_REG_DX, size);
	scf_x64_OpCode_t* 	mul_OpCode  = NULL;

	if (scf_type_is_signed(src0->dag_node->var->type)
			|| scf_type_is_signed(src1->dag_node->var->type))
		mul_OpCode = x64_find_OpCode(SCF_X64_IMUL,  size, size, SCF_X64_E);
	else
		mul_OpCode = x64_find_OpCode(SCF_X64_MUL,  size, size, SCF_X64_E);

	if (dst->dag_node->color > 0) {
		rd = x64_find_register_color(dst->dag_node->color);

		// ax means: al, ax, eax, rax, they have same id
		// dx means: dl, dx, edx, rdx, they have same id
		// if dst not in ax, save ax
		// if dst not in dx, save dx
		// if dst not in ax & dx, save ax & dx
		if (rd->id != ax->id) {
			inst = x64_make_inst_G(push_OpCode, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}

		if (rd->id != dx->id) {
			inst = x64_make_inst_G(push_OpCode, dx);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		// if dst in momery, save ax & dx
		inst = x64_make_inst_G(push_OpCode, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_G(push_OpCode, dx);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	if (src0->dag_node->color > 0) {
		rs0 = x64_find_register_color(src0->dag_node->color);

		if (src1->dag_node->color > 0) {
			rs1 = x64_find_register_color(src1->dag_node->color);

			// ax & dx already saved above
			if (rs0->id == ax->id) {
				inst = x64_make_inst_G(mul_OpCode, rs1);
				X64_INST_ADD_CHECK(c->instructions, inst);

			} else if (rs1->id == ax->id) {
				inst = x64_make_inst_G(mul_OpCode, rs0);
				X64_INST_ADD_CHECK(c->instructions, inst);

			} else {
				inst = x64_make_inst_G2E(mov_OpCode, ax, rs0);
				X64_INST_ADD_CHECK(c->instructions, inst);

				inst = x64_make_inst_G(mul_OpCode, rs1);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}
		} else {
			if (rs0->id != ax->id) {
				inst = x64_make_inst_G2E(mov_OpCode, ax, rs0);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}

			if (0 == src1->dag_node->color) {
				assert(scf_variable_const(src1->dag_node->var));

				// dx already saved above
				inst = x64_make_inst_I2G(mov_OpCode, dx, (uint8_t*)&src1->dag_node->var->data, size);
				X64_INST_ADD_CHECK(c->instructions, inst);

				inst = x64_make_inst_G(mul_OpCode, dx);
				X64_INST_ADD_CHECK(c->instructions, inst);
			} else {
				scf_rela_t* rela = NULL;

				inst = x64_make_inst_M(&rela, mul_OpCode, src1->dag_node->var, NULL);
				X64_INST_ADD_CHECK(c->instructions, inst);
				X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->dag_node->var, NULL);
			}
		}
	} else {
		if (src1->dag_node->color > 0) {
			rs1 = x64_find_register_color(src1->dag_node->color);

			if (rs1->id != ax->id) {
				inst = x64_make_inst_G2E(mov_OpCode, ax, rs1);
				X64_INST_ADD_CHECK(c->instructions, inst);
			}

			if (0 == src0->dag_node->color) {
				assert(scf_variable_const(src0->dag_node->var));

				inst = x64_make_inst_I2G(mov_OpCode, dx, (uint8_t*)&src0->dag_node->var->data, size);
				X64_INST_ADD_CHECK(c->instructions, inst);

				inst = x64_make_inst_G(mul_OpCode, dx);
				X64_INST_ADD_CHECK(c->instructions, inst);
			} else {
				scf_rela_t* rela = NULL;

				inst = x64_make_inst_M(&rela, mul_OpCode, src0->dag_node->var, NULL);
				X64_INST_ADD_CHECK(c->instructions, inst);
				X64_RELA_ADD_CHECK(f->data_relas, rela, c, src0->dag_node->var, NULL);
			}
		} else {
			if (0 == src0->dag_node->color) {
				assert(scf_variable_const(src0->dag_node->var));

				inst = x64_make_inst_I2G(mov_OpCode, ax, (uint8_t*)&src0->dag_node->var->data, size);
				X64_INST_ADD_CHECK(c->instructions, inst);
			} else {
				scf_rela_t* rela = NULL;

				inst = x64_make_inst_M2G(&rela, mov_OpCode, ax, NULL, src0->dag_node->var);
				X64_INST_ADD_CHECK(c->instructions, inst);
				X64_RELA_ADD_CHECK(f->data_relas, rela, c, src0->dag_node->var, NULL);
			}

			if (0 == src1->dag_node->color) {
				assert(scf_variable_const(src1->dag_node->var));

				inst = x64_make_inst_I2G(mov_OpCode, dx, (uint8_t*)&src1->dag_node->var->data, size);
				X64_INST_ADD_CHECK(c->instructions, inst);

				inst = x64_make_inst_G(mul_OpCode, dx);
				X64_INST_ADD_CHECK(c->instructions, inst);
			} else {
				scf_rela_t* rela = NULL;

				inst = x64_make_inst_M(&rela, mul_OpCode, src1->dag_node->var, NULL);
				X64_INST_ADD_CHECK(c->instructions, inst);
				X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->dag_node->var, NULL);
			}
		}
	}

	if (rd) {
		if (rd->id != ax->id) {
			inst = x64_make_inst_G2E(mov_OpCode, rd, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);

			inst = x64_make_inst_G(pop_OpCode, ax);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}

		if (rd->id != dx->id) {
			inst = x64_make_inst_G(pop_OpCode, dx);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}
	} else {
		scf_rela_t* rela = NULL;
		inst = x64_make_inst_G2M(&rela, mov_OpCode, dst->dag_node->var, NULL, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->dag_node->var, NULL);

		inst = x64_make_inst_G(pop_OpCode, dx);
		X64_INST_ADD_CHECK(c->instructions, inst);

		inst = x64_make_inst_G(pop_OpCode, ax);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}
	return 0;
}

static int _x64_inst_op3_rrm(int OpCode_type, scf_register_x64_t* rd, scf_register_x64_t* rs0, scf_dag_node_t* src1, scf_3ac_code_t* c, scf_function_t* f)
{
	int size = rs0->bytes;

	scf_x64_OpCode_t* 	op;
	scf_instruction_t*  inst;
	scf_x64_OpCode_t* 	mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);
	scf_rela_t*         rela = NULL;

	if (rs0->id != rd->id) {
		inst = x64_make_inst_G2E(mov, rd, rs0);
		X64_INST_ADD_CHECK(c->instructions, inst);
	}

	if (0 == src1->color) {
		assert(scf_variable_const(src1->var));

		op   = x64_find_OpCode(OpCode_type, size, size, SCF_X64_I2E);
		inst = x64_make_inst_I2E(op, rd, (uint8_t*)&src1->var->data, size);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		op   = x64_find_OpCode(OpCode_type, size, size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, op, rd, NULL, src1->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->var, NULL);
	}

	return 0;
}

static int _x64_inst_op3_rrr(int OpCode_type, scf_register_x64_t* rd, scf_register_x64_t* rs0, scf_register_x64_t* rs1, scf_vector_t* instructions)
{
	int size = rs0->bytes;

	scf_x64_OpCode_t* 	op  = x64_find_OpCode(OpCode_type, size, size, SCF_X64_G2E);
	scf_x64_OpCode_t* 	mov = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_G2E);
	scf_instruction_t*  inst;

	if (rd->id == rs0->id) {
		inst = x64_make_inst_G2E(op, rd, rs1);
		X64_INST_ADD_CHECK(instructions, inst);

	} else if (rd->id == rs1->id) {
		inst = x64_make_inst_G2E(op, rd, rs0);
		X64_INST_ADD_CHECK(instructions, inst);

		if (SCF_X64_SUB == OpCode_type) {
			op   = x64_find_OpCode(SCF_X64_NEG, size, size, SCF_X64_E);
			inst = x64_make_inst_E(op, rd);
			X64_INST_ADD_CHECK(instructions, inst);
		}
	} else {
		inst = x64_make_inst_G2E(mov, rd, rs0);
		X64_INST_ADD_CHECK(instructions, inst);

		inst = x64_make_inst_G2E(op, rd, rs1);
		X64_INST_ADD_CHECK(instructions, inst);
	}
	return 0;
}

static int _x64_inst_op3_rmm(int OpCode_type, scf_register_x64_t* rd, scf_dag_node_t* src0, scf_dag_node_t* src1, scf_3ac_code_t* c, scf_function_t* f)
{
	int size = src0->var->size;

	scf_x64_OpCode_t* 	op;
	scf_x64_OpCode_t* 	mov;
	scf_instruction_t*  inst;

	if (0 == src0->color) {
		assert(scf_variable_const(src0->var));

		mov  = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, rd, (uint8_t*)&src0->var->data, size);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		mov  = x64_find_OpCode(SCF_X64_MOV, size, size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, mov, rd, NULL, src0->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src0->var, NULL);
	}

	if (0 == src1->color) {
		assert(scf_variable_const(src1->var));

		op   = x64_find_OpCode(OpCode_type, size, size, SCF_X64_I2E);
		inst = x64_make_inst_I2E(op, rd, (uint8_t*)&src1->var->data, size);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		scf_rela_t* rela = NULL;

		op   = x64_find_OpCode(OpCode_type, size, size, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, op, rd, NULL, src1->var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->var, NULL);
	}

	return 0;
}

static int _x64_inst_add_sub(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_x64_context_t* x64  = ctx->priv;
	scf_function_t*    f    = x64->f;
	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];
	scf_3ac_operand_t* dst  = c->dst;

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (src0->dag_node->var->size != src1->dag_node->var->size)
		return -EINVAL;

	if (src0->dag_node->var->size != dst->dag_node->var->size)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int size = src0->dag_node->var->size;
	int ret = 0;

	scf_register_x64_t* rs0  = NULL;
	scf_register_x64_t* rs1  = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;

	scf_x64_OpCode_t* 	mov  = x64_find_OpCode(SCF_X64_MOV,  size, size, SCF_X64_G2E);

	ret = _x64_inst_select_reg(&rd, c->dst->dag_node, c, f, 0);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	assert(c->dst->dag_node->color > 0);

	if (src0->dag_node->color > 0) {
		rs0 = x64_find_register_color(src0->dag_node->color);

		if (src1->dag_node->color > 0) {
			rs1 = x64_find_register_color(src1->dag_node->color);
			ret = _x64_inst_op3_rrr(OpCode_type, rd, rs0, rs1, c->instructions);
		} else {
			ret = _x64_inst_op3_rrm(OpCode_type, rd, rs0, src1->dag_node, c, f);
		}
	} else if (src1->dag_node->color > 0) {
		rs1 = x64_find_register_color(src1->dag_node->color);
		ret = _x64_inst_op3_rrm(OpCode_type, rd, rs1, src0->dag_node, c, f);
	} else
		ret = _x64_inst_op3_rmm(OpCode_type, rd, src0->dag_node, src1->dag_node, c, f);

	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return 0;
}

static int _x64_inst_add_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_add_sub(ctx, c, SCF_X64_ADD);
}

static int _x64_inst_sub_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_add_sub(ctx, c, SCF_X64_SUB);
}

static int _x64_inst_cmp(scf_dag_node_t* src0, scf_dag_node_t* src1, scf_3ac_code_t* c, scf_function_t* f)
{
	if (0 == src0->color) {
		scf_loge("src0 should be a var\n");
		if (src0->var->w)
			scf_loge("src0: '%s'\n", src0->var->w->text->data);
		else
			scf_loge("src0: v_%#lx\n", 0xffff & (uintptr_t)src0->var);
		return -EINVAL;
	}

	scf_x64_OpCode_t*   cmp;
	scf_instruction_t*  inst;
	scf_register_x64_t* rs1;
	scf_register_x64_t* rs0  = NULL;
	scf_rela_t*         rela = NULL;

	int ret = _x64_inst_select_reg(&rs0, src0, c, f, 1);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	assert(src0->color > 0);

	if (src1->color > 0) {
		rs1  = x64_find_register_color(src1->color);
		cmp  = x64_find_OpCode(SCF_X64_CMP, rs0->bytes, rs1->bytes, SCF_X64_G2E);
		inst = x64_make_inst_G2E(cmp, rs0, rs1);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else if (0 == src1->color) {
		cmp  = x64_find_OpCode(SCF_X64_CMP, rs0->bytes, src1->var->size, SCF_X64_I2E);
		inst = x64_make_inst_I2E(cmp, rs0, (uint8_t*)&src1->var->data, src1->var->size);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		cmp  = x64_find_OpCode(SCF_X64_CMP, rs0->bytes, src1->var->size, SCF_X64_G2E);
		inst = x64_make_inst_G2M(&rela, cmp, src1->var, NULL, rs0);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, src1->var, NULL);
	}

	return 0;
}

static int _x64_inst_cmp_set(int setcc_type, scf_dag_node_t* dst, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_x64_OpCode_t* setcc = x64_find_OpCode(setcc_type, 1,1, SCF_X64_E);
	if (!setcc)
		return -EINVAL;

	scf_instruction_t*  inst;
	scf_register_x64_t* rd;
	scf_rela_t*         rela = NULL;

	if (dst->color > 0) {
		rd   = x64_find_register_color(dst->color);
		inst = x64_make_inst_E(setcc, rd);
		X64_INST_ADD_CHECK(c->instructions, inst);

	} else {
		inst = x64_make_inst_M(&rela, setcc, dst->var, NULL);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, dst->var, NULL);
	}
	return 0;
}

static int _x64_inst_teq_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->srcs || c->srcs->size != 1)
		return -EINVAL;

	scf_3ac_operand_t* src = c->srcs->data[0];

	if (!src || !src->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	scf_loge("\n");
	return -1;
}

static int _x64_inst_cmp_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_x64_context_t* x64  = ctx->priv;
	scf_function_t*    f    = x64->f;
	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (src0->dag_node->var->size != src1->dag_node->var->size)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	return _x64_inst_cmp(src0->dag_node, src1->dag_node, c, f);
}

static int _x64_inst_eq_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETZ, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_ne_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETNZ, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_gt_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETG, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_ge_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETGE, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_lt_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETL, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_le_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	scf_x64_context_t* x64 = ctx->priv;
	scf_function_t*    f   = x64->f;

	int ret = _x64_inst_cmp_handler(ctx, c);
	if (ret < 0)
		return ret;

	ret = _x64_inst_cmp_set(SCF_X64_SETLE, c->dst->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

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

	if (src->dag_node->var->size < dst->dag_node->var->size)
		return _x64_inst_op_movx(dst->dag_node, src->dag_node, c, f);

	return _x64_inst_op2(SCF_X64_MOV, dst->dag_node, src->dag_node, c, f);
}

static int _x64_inst_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c)
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

	int ret = _x64_inst_op2(SCF_X64_MOV, c->dst->dag_node, src->dag_node, c, f);
	if (ret < 0)
		return ret;
	return 0;
}

static int _x64_inst_return_handler(scf_native_t* ctx, scf_3ac_code_t* c)
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

	scf_register_x64_t*	rd	   = NULL;
	scf_register_x64_t*	rs	   = NULL;
	scf_x64_OpCode_t*   mov    = NULL;
	scf_x64_OpCode_t*   ret    = NULL;
	scf_instruction_t*  inst   = NULL;
	scf_rela_t*         rela   = NULL;
	scf_variable_t*		var    = src->dag_node->var;

	if (8 == var->size)
		rd = x64_find_register("rax");
	else
		rd = x64_find_register("eax");

	if (0 == src->dag_node->color) {
		if (rd->bytes > var->size)
			scf_variable_extend_bytes(var, rd->bytes);

		mov = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_I2G);
		inst = x64_make_inst_I2G(mov, rd, (uint8_t*)&var->data, rd->bytes);
		X64_INST_ADD_CHECK(c->instructions, inst);
		goto end;
	}

	if (rd->bytes > var->size) {
		if (scf_type_is_signed(var->type))
			mov = x64_find_OpCode(SCF_X64_MOVSX, var->size, rd->bytes, SCF_X64_E2G);
		else
			mov = x64_find_OpCode(SCF_X64_MOVZX, var->size, rd->bytes, SCF_X64_E2G);
	} else {
		mov = x64_find_OpCode(SCF_X64_MOV, rd->bytes, rd->bytes, SCF_X64_E2G);
	}

	if (src->dag_node->color > 0) {
		rs   = x64_find_register_color(src->dag_node->color);
		inst = x64_make_inst_E2G(mov, rd, rs);
		X64_INST_ADD_CHECK(c->instructions, inst);
	} else {
		inst = x64_make_inst_M2G(&rela, mov, rd, NULL, var);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, var, NULL);
	}

end:
#if 1
	ret = x64_find_OpCode(SCF_X64_RET, 8, 8, SCF_X64_G);
	inst = x64_make_inst_G(ret, rd);
	X64_INST_ADD_CHECK(c->instructions, inst);
#endif
	return 0;
}

static int _x64_inst_jmp(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->dst || !c->dst->code)
		return -EINVAL;

	c->instructions = scf_vector_alloc();
	if (!c->instructions)
		return -ENOMEM;

	uint32_t offset = 0;

	scf_x64_OpCode_t*  j    = x64_find_OpCode(OpCode_type, 1,1, SCF_X64_I);
	scf_instruction_t* inst = x64_make_inst_I(j, (uint8_t*)&offset, sizeof(offset));

	X64_INST_ADD_CHECK(c->instructions, inst);
	return 0;
}

static int _x64_inst_goto_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JMP);
}

static int _x64_inst_jz_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JZ);
}

static int _x64_inst_jnz_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JNZ);
}

static int _x64_inst_jgt_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JG);
}

static int _x64_inst_jge_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JGE);
}

static int _x64_inst_jlt_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JL);
}

static int _x64_inst_jle_handler(scf_native_t* ctx, scf_3ac_code_t* c)
{
	return _x64_inst_jmp(ctx, c, SCF_X64_JLE);
}

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

	scf_register_x64_t* r   = x64_find_register_color(dn->color);
	scf_x64_OpCode_t*   mov = x64_find_OpCode(SCF_X64_MOV, dn->var->size, r->bytes, SCF_X64_E2G);
	if (!mov)
		return -EINVAL;

//	scf_logw("dn->color: %ld, dn->var->w: %s\n", dn->color, dn->var->w->text->data);
	scf_rela_t*         rela = NULL;
	scf_instruction_t*  inst = x64_make_inst_M2G(&rela, mov, r, NULL, dn->var);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, dn->var, NULL);
	return 0;
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

	scf_register_x64_t* r   = x64_find_register_color(dn->color);
	scf_x64_OpCode_t*   mov = x64_find_OpCode(SCF_X64_MOV, dn->var->size, r->bytes, SCF_X64_G2E);
	if (!mov)
		return -EINVAL;

	scf_rela_t*         rela = NULL;
	scf_instruction_t*  inst = x64_make_inst_G2M(&rela, mov, dn->var, NULL, r);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, dn->var, NULL);
	return 0;
}

static x64_inst_handler_t x64_inst_handlers[] = {

	{SCF_OP_CALL,			_x64_inst_call_handler},
	{SCF_OP_ARRAY_INDEX, 	_x64_inst_array_index_handler},

	{SCF_OP_TYPE_CAST,      _x64_inst_cast_handler},
	{SCF_OP_LOGIC_NOT, 		_x64_inst_logic_not_handler},
	{SCF_OP_NEG, 			_x64_inst_neg_handler},
	{SCF_OP_POSITIVE, 		_x64_inst_positive_handler},

	{SCF_OP_DEREFERENCE, 	_x64_inst_dereference_handler},
	{SCF_OP_ADDRESS_OF, 	_x64_inst_address_of_handler},

	{SCF_OP_MUL, 			_x64_inst_mul_handler},
	{SCF_OP_DIV, 			_x64_inst_div_handler},
	{SCF_OP_ADD, 			_x64_inst_add_handler},
	{SCF_OP_SUB, 			_x64_inst_sub_handler},

	{SCF_OP_3AC_TEQ,        _x64_inst_teq_handler},
	{SCF_OP_3AC_CMP,        _x64_inst_cmp_handler},

	{SCF_OP_EQ, 			_x64_inst_eq_handler},
	{SCF_OP_NE, 			_x64_inst_ne_handler},
	{SCF_OP_GT, 			_x64_inst_gt_handler},
	{SCF_OP_GE,             _x64_inst_ge_handler},
	{SCF_OP_LT,             _x64_inst_lt_handler},
	{SCF_OP_LE,             _x64_inst_le_handler},

	{SCF_OP_ASSIGN, 		_x64_inst_assign_handler},

	{SCF_OP_RETURN, 		_x64_inst_return_handler},
	{SCF_OP_GOTO, 			_x64_inst_goto_handler},

	{SCF_OP_3AC_JZ, 		_x64_inst_jz_handler},
	{SCF_OP_3AC_JNZ, 		_x64_inst_jnz_handler},
	{SCF_OP_3AC_JGT, 		_x64_inst_jgt_handler},
	{SCF_OP_3AC_JGE, 		_x64_inst_jge_handler},
	{SCF_OP_3AC_JLT, 		_x64_inst_jlt_handler},
	{SCF_OP_3AC_JLE, 		_x64_inst_jle_handler},

	{SCF_OP_3AC_SAVE,       _x64_inst_save_handler},
	{SCF_OP_3AC_LOAD,       _x64_inst_load_handler},
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
