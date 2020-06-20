#include"scf_native_x64.h"

static scf_register_x64_t* _x64_register_get_by_color(scf_x64_context_t* x64, int color, int size)
{
	scf_register_x64_t* r = x64->colored_registers[color - 1];
	if (!r) {
		printf("%s(),%d, color: %d, size: %d\n", __func__, __LINE__, color, size);
		r = (scf_register_x64_t*)_x64_find_free_register(x64, size);
		assert(r);
		x64->colored_registers[color - 1] = r;
		_x64_register_make_busy(x64, r);

		printf("%s(),%d, r: %s, r->bytes: %d, size: %d\n", __func__, __LINE__, r->name, r->bytes, size);
	}

	printf("%s(),%d, r: %s, r->bytes: %d, size: %d\n", __func__, __LINE__, r->name, r->bytes, size);
	assert(r->bytes == size);
	return r;
}

static int _x64_op_mov_E2G(scf_native_context_t* ctx, scf_register_x64_t* r_dst, scf_dag_node_t* src, scf_vector_t* instructions)
{
	scf_x64_context_t* x64 = ctx->priv;

	assert(src);

	scf_x64_OpCode_t*	OpCode	= NULL;
	scf_register_x64_t* rs		= NULL;
	scf_instruction_t*  inst	= NULL;

	if (0 == src->color) {
		assert(src->var->const_literal_flag);

		OpCode = _x64_find_OpCode(SCF_X64_MOV, src->var->size, src->var->size, SCF_X64_I2G);
		if (OpCode) {
			inst = _x64_make_instruction_I2G(OpCode, r_dst, src->var);

		} else {
			OpCode = _x64_find_OpCode(SCF_X64_MOV, src->var->size, src->var->size, SCF_X64_I2E);
			assert(OpCode);

			inst = _x64_make_instruction_I2E_reg(OpCode, r_dst, src->var);
		}
	} else if (src->color > 0) {
		rs = _x64_register_get_by_color(x64, src->color, src->var->size);

		OpCode = _x64_find_OpCode(SCF_X64_MOV, src->var->size, src->var->size, SCF_X64_G2E);
		assert(OpCode);

		inst = _x64_make_instruction_G2G(OpCode, r_dst, rs);
	} else {
		OpCode = _x64_find_OpCode(SCF_X64_MOV, src->var->size, src->var->size, SCF_X64_E2G);
		assert(OpCode);

		inst = _x64_make_instruction_E2G(OpCode, r_dst, src->var);
	}

	scf_vector_add(instructions, inst);
	return 0;
}

static int _x64_op_2(scf_native_context_t* ctx, int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_vector_t* instructions)
{
	scf_x64_context_t* x64 = ctx->priv;

	assert(src);
	assert(dst);

	assert(0 != dst->color);

	scf_x64_OpCode_t*	OpCode	= NULL;
	scf_register_x64_t* rs		= NULL;
	scf_register_x64_t* rd		= NULL;
	scf_instruction_t*  inst	= NULL;

	if (0 == src->color) {
		assert(src->var->const_literal_flag);

		if (dst->color > 0) {
			printf("%s(),%d, OpCode_type: %d, size: %d, size: %d\n", __func__, __LINE__, OpCode_type, src->var->size, dst->var->size);

			rd   = _x64_register_get_by_color(x64, dst->color, dst->var->size);

			OpCode = _x64_find_OpCode(OpCode_type, src->var->size, dst->var->size, SCF_X64_I2G);
			if (OpCode) {
				inst = _x64_make_instruction_I2G(OpCode, rd, src->var);

			} else {
				OpCode = _x64_find_OpCode(OpCode_type, src->var->size,
						dst->var->size, SCF_X64_I2E);
				assert(OpCode);

				inst = _x64_make_instruction_I2E_reg(OpCode, rd, src->var);
			}
		} else {
			OpCode = _x64_find_OpCode(OpCode_type, src->var->size, dst->var->size, SCF_X64_I2E);
			assert(OpCode);

			inst = _x64_make_instruction_I2E_var(OpCode, dst->var, src->var);
		}
	} else if (src->color > 0) {
		rs = _x64_register_get_by_color(x64, src->color, src->var->size);

		OpCode = _x64_find_OpCode(OpCode_type, src->var->size, dst->var->size, SCF_X64_G2E);
		assert(OpCode);

		if (dst->color > 0) {
			rd   = _x64_register_get_by_color(x64, dst->color, dst->var->size);
			inst = _x64_make_instruction_G2G(OpCode, rd, rs);
		} else {
			inst = _x64_make_instruction_G2E(OpCode, dst->var, rs);
		}
	} else {
		if (dst->color > 0) {
			rd = _x64_register_get_by_color(x64, dst->color, dst->var->size);

			OpCode = _x64_find_OpCode(OpCode_type, src->var->size, dst->var->size, SCF_X64_E2G);
			assert(OpCode);

			inst = _x64_make_instruction_E2G(OpCode, rd, src->var);
		} else {
			assert(src->var->size == dst->var->size);

			scf_register_x64_t* r;
			if (8 == src->var->size)
				r = (scf_register_x64_t*)_x64_find_register("rax");
			else
				r = (scf_register_x64_t*)_x64_find_register("eax");

			scf_x64_OpCode_t* push    = _x64_find_OpCode(SCF_X64_PUSH, 8, 8, SCF_X64_G);
			scf_x64_OpCode_t* pop     = _x64_find_OpCode(SCF_X64_POP, 8, 8, SCF_X64_G);
			scf_x64_OpCode_t* mov_E2G = _x64_find_OpCode(SCF_X64_MOV, src->var->size, r->bytes, SCF_X64_E2G);
			scf_x64_OpCode_t* op_G2E = _x64_find_OpCode(OpCode_type, src->var->size, r->bytes, SCF_X64_G2E);

			scf_instruction_t* inst_push = _x64_make_instruction_G(push, r);
			scf_instruction_t* inst_E2G  = _x64_make_instruction_E2G(mov_E2G, r, src->var);
			scf_instruction_t* inst_G2E  = _x64_make_instruction_G2E(op_G2E, dst->var, r);
			scf_instruction_t* inst_pop  = _x64_make_instruction_G(pop, r);

			scf_vector_add(instructions, inst_push);
			scf_vector_add(instructions, inst_E2G);
			scf_vector_add(instructions, inst_G2E);
			scf_vector_add(instructions, inst_pop);
			return 0;
		}
	}

	scf_vector_add(instructions, inst);
	return 0;
}

static int _x64_3ac_call_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(_3ac->srcs->size >= 1);

	scf_3ac_operand_t* dst	= _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_function_t*    f	= (scf_function_t*)src0->node;

	printf("%s(),%d, call %s\n", __func__, __LINE__, f->node.w->text->data);

	scf_x64_OpCode_t*  call = _x64_find_OpCode(SCF_X64_CALL, 4,4, SCF_X64_I);
	scf_x64_OpCode_t*  push = _x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t*  pop  = _x64_find_OpCode(SCF_X64_POP, 8,8, SCF_X64_G);
	scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
	assert(inst);

	if (!_3ac->instructions)
		_3ac->instructions = scf_vector_alloc();

	// ABI: rdi rsi rdx rcx r8 r9
	char* regs[] = {"rdi", "rsi", "rdx", "rcx"};

	int i;
	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
		scf_register_x64_t* r    = (scf_register_x64_t*)_x64_find_register(regs[i]);
		scf_instruction_t*  save = _x64_make_instruction_G(push, r);
	}

	inst->OpCode 			= (scf_OpCode_t*)call;
	inst->code[inst->len++] = call->OpCodes[0];

	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
	inst->code[inst->len++] = 0;
	scf_vector_add(_3ac->instructions, inst);

	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
		scf_register_x64_t* r    = (scf_register_x64_t*)_x64_find_register(regs[i]);
		scf_instruction_t*  save = _x64_make_instruction_G(push, r);
	}
	return 0;
}

static int _x64_3ac_array_index_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_3ac_logic_not_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}
static int _x64_3ac_neg_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_3ac_positive_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}
static int _x64_3ac_dereference_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_3ac_address_of_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}
static int _x64_3ac_div_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}
static int _x64_3ac_mul_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	scf_x64_context_t* x64 = ctx->priv;

	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(dst->dag_node->color != 0);
	assert(src0->dag_node->var->size == src1->dag_node->var->size);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	scf_register_x64_t* rs0  = NULL;
	scf_register_x64_t* rs1  = NULL;
	scf_register_x64_t* rd   = NULL;
	scf_instruction_t*  inst = NULL;

	scf_x64_OpCode_t*	push_OpCode = _x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* 	pop_OpCode  = _x64_find_OpCode(SCF_X64_POP, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* 	mul_OpCode  = _x64_find_OpCode(SCF_X64_MUL, src0->dag_node->var->size, src0->dag_node->var->size, SCF_X64_E);
	scf_x64_OpCode_t* 	mov_OpCode  = _x64_find_OpCode(SCF_X64_MOV, dst->dag_node->var->size, dst->dag_node->var->size, SCF_X64_G2E);
	scf_register_x64_t* rax 		= (scf_register_x64_t*)_x64_find_register("rax");
	scf_register_x64_t* rdx 		= (scf_register_x64_t*)_x64_find_register("rdx");

	if (dst->dag_node->color > 0) {
		rd = _x64_register_get_by_color(x64, dst->dag_node->color, dst->dag_node->var->size);

		if (rd->id != rax->id) {
			scf_vector_add(_3ac->instructions, _x64_make_instruction_G(push_OpCode, rax));
		}

		if (rd->id != rdx->id) {
			scf_vector_add(_3ac->instructions, _x64_make_instruction_G(push_OpCode, rdx));
		}
	} else {
		scf_vector_add(_3ac->instructions, _x64_make_instruction_G(push_OpCode, rax));
		scf_vector_add(_3ac->instructions, _x64_make_instruction_G(push_OpCode, rdx));
	}

	if (src0->dag_node->color > 0) {
		rs0 = _x64_register_get_by_color(x64, src0->dag_node->color, src0->dag_node->var->size);

		if (src1->dag_node->color > 0) {
			rs1 = _x64_register_get_by_color(x64, src1->dag_node->color, src1->dag_node->var->size);

			if (rs0->id == rax->id) {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rs1));

			} else if (rs1->id == rax->id) {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rs0));

			} else {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G2G(mov_OpCode, rax, rs0));
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rs1));
			}
		} else {
			if (rs0->id != rax->id) {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G2G(mov_OpCode, rax, rs0));
			}

			if (0 == src1->dag_node->color) {
				assert(src1->dag_node->var->const_literal_flag);
				scf_vector_add(_3ac->instructions, _x64_make_instruction_I2G(mov_OpCode, rdx, src1->dag_node->var));
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rdx));
			} else {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_E_var(mul_OpCode, src1->dag_node->var));
			}
		}
	} else {
		if (src1->dag_node->color > 0) {
			rs1 = _x64_register_get_by_color(x64, src1->dag_node->color, src1->dag_node->var->size);

			if (rs1->id != rax->id) {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G2G(mov_OpCode, rax, rs1));
			}

			if (0 == src0->dag_node->color) {
				assert(src0->dag_node->var->const_literal_flag);
				scf_vector_add(_3ac->instructions, _x64_make_instruction_I2G(mov_OpCode, rdx, src0->dag_node->var));
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rdx));
			} else {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_E_var(mul_OpCode, src0->dag_node->var));
			}
		} else {
			if (0 == src0->dag_node->color) {
				assert(src0->dag_node->var->const_literal_flag);
				scf_vector_add(_3ac->instructions, _x64_make_instruction_I2G(mov_OpCode, rax, src0->dag_node->var));
			} else {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_E2G(mov_OpCode, rax, src0->dag_node->var));
			}

			if (0 == src1->dag_node->color) {
				assert(src1->dag_node->var->const_literal_flag);
				scf_vector_add(_3ac->instructions, _x64_make_instruction_I2G(mov_OpCode, rdx, src1->dag_node->var));
				scf_vector_add(_3ac->instructions, _x64_make_instruction_G(mul_OpCode, rdx));
			} else {
				scf_vector_add(_3ac->instructions, _x64_make_instruction_E_var(mul_OpCode, src1->dag_node->var));
			}
		}
	}

	printf("%s(),%d, rax: %d, rdx: %d\n", __func__, __LINE__, rax->id, rdx->id);
	if (rd)
		printf("%s(),%d, rd: %d\n", __func__, __LINE__, rd->id);
	if (rs0)
		printf("%s(),%d, rs0: %d\n", __func__, __LINE__, rs0->id);
	if (rs1)
		printf("%s(),%d, rs1: %d\n", __func__, __LINE__, rs1->id);

	if (rd) {
		if (rd->id != rax->id) {
			scf_vector_add(_3ac->instructions, _x64_make_instruction_G2G(mov_OpCode, rd, rax));
			scf_vector_add(_3ac->instructions, _x64_make_instruction_G(pop_OpCode, rax));
		}

		if (rd->id != rdx->id) {
			scf_vector_add(_3ac->instructions, _x64_make_instruction_G(pop_OpCode, rdx));
		}
	} else {
		scf_vector_add(_3ac->instructions, _x64_make_instruction_G2E(mov_OpCode, dst->dag_node->var, rax));
		scf_vector_add(_3ac->instructions, _x64_make_instruction_G(pop_OpCode, rdx));
		scf_vector_add(_3ac->instructions, _x64_make_instruction_G(pop_OpCode, rax));
	}
	return 0;
}

static int _x64_3ac_add_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(dst->dag_node->color != 0);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	_x64_op_2(ctx, SCF_X64_MOV, dst->dag_node, src0->dag_node, _3ac->instructions);
	_x64_op_2(ctx, SCF_X64_ADD, dst->dag_node, src1->dag_node, _3ac->instructions);

	return 0;
}

static int _x64_3ac_sub_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(dst->dag_node->color != 0);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	_x64_op_2(ctx, SCF_X64_MOV, dst->dag_node, src0->dag_node, _3ac->instructions);
	_x64_op_2(ctx, SCF_X64_SUB, dst->dag_node, src1->dag_node, _3ac->instructions);

	return 0;
}

static int _x64_clear_var(scf_native_context_t* ctx, scf_dag_node_t* dst, scf_vector_t* instructions)
{
	scf_x64_context_t*  x64 = ctx->priv;

	if (dst->color > 0) {
		scf_register_x64_t* rd  = _x64_register_get_by_color(x64, dst->color, dst->var->size);
		scf_x64_OpCode_t*   xor = _x64_find_OpCode(SCF_X64_XOR, dst->var->size, dst->var->size, SCF_X64_G2E);

		scf_vector_add(instructions, _x64_make_instruction_G2G(xor, rd, rd));
	} else {
		assert(!dst->var->const_literal_flag);
		scf_x64_OpCode_t* push = _x64_find_OpCode(SCF_X64_PUSH, 8, 8, SCF_X64_G);
		scf_x64_OpCode_t* pop  = _x64_find_OpCode(SCF_X64_POP,  8, 8, SCF_X64_G);
		scf_x64_OpCode_t* xor  = _x64_find_OpCode(SCF_X64_XOR,  8, 8, SCF_X64_G2E);
		scf_x64_OpCode_t* mov  = _x64_find_OpCode(SCF_X64_MOV,  dst->var->size, dst->var->size, SCF_X64_G2E);

		scf_register_x64_t* r;
		if (8 == dst->var->size)
			r = (scf_register_x64_t*)_x64_find_register("rax");
		else
			r = (scf_register_x64_t*)_x64_find_register("eax");

		scf_vector_add(instructions, _x64_make_instruction_G(push, r));
		scf_vector_add(instructions, _x64_make_instruction_G2G(xor, r, r));
		scf_vector_add(instructions, _x64_make_instruction_G2E(mov, dst->var, r));
		scf_vector_add(instructions, _x64_make_instruction_G(pop, r));
	}
	return 0;
}

static int _x64_cmp_set(scf_native_context_t* ctx, int setcc_type, scf_dag_node_t* dst, scf_vector_t* instructions)
{
	scf_x64_context_t*  x64 = ctx->priv;

	scf_x64_OpCode_t*   setcc = _x64_find_OpCode(setcc_type, 1,1, SCF_X64_E);
	scf_register_x64_t* rd   = NULL;

	if (dst->color > 0) {
		rd = _x64_register_get_by_color(x64, dst->color, dst->var->size);

		scf_instruction_t* inst = calloc(1, sizeof(scf_instruction_t));
		assert(inst);

		inst->OpCode = (scf_OpCode_t*)setcc;

		if (rd->bytes > 1 && rd->id >= SCF_X64_RM_ESI) {
			inst->code[inst->len++] = SCF_X64_REX_INIT;
		}

		int i;
		for (i = 0; i < setcc->nb_OpCodes; i++)
			inst->code[inst->len++] = setcc->OpCodes[i];

		uint8_t ModRM = 0;
		if (setcc->ModRM_OpCode_used)
			scf_ModRM_setReg(&ModRM, setcc->ModRM_OpCode);
		scf_ModRM_setRM(&ModRM, rd->id);
		scf_ModRM_setMod(&ModRM, SCF_X64_MOD_REGISTER);

		inst->code[inst->len++] = ModRM;

		scf_vector_add(instructions, inst);
	} else {
		assert(!dst->var->const_literal_flag);

		scf_vector_add(instructions, _x64_make_instruction_E_var(setcc, dst->var));
	}
}

static int _x64_3ac_eq_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(src0->dag_node->var->size == src1->dag_node->var->size);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	if (dst->dag_node->var->size > 1)
		_x64_clear_var(ctx, dst->dag_node, _3ac->instructions);

	_x64_op_2(ctx, SCF_X64_CMP, src1->dag_node, src0->dag_node, _3ac->instructions);
	_x64_cmp_set(ctx, SCF_X64_SETZ, dst->dag_node, _3ac->instructions);
	return 0;
}

static int _x64_3ac_gt_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(src0->dag_node->var->size == src1->dag_node->var->size);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	if (dst->dag_node->var->size > 1)
		_x64_clear_var(ctx, dst->dag_node, _3ac->instructions);

	_x64_op_2(ctx, SCF_X64_CMP, src1->dag_node, src0->dag_node, _3ac->instructions);
	_x64_cmp_set(ctx, SCF_X64_SETG, dst->dag_node, _3ac->instructions);
	return 0;
}

static int _x64_3ac_lt_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(_3ac->dst && _3ac->dst->node);
	assert(2 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src0 = _3ac->srcs->data[0];
	scf_3ac_operand_t* src1 = _3ac->srcs->data[1];
	assert(src0 && src0->node);
	assert(src1 && src1->node);
	assert(dst && dst->node);
	assert(src0->dag_node->var->size == src1->dag_node->var->size);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	if (dst->dag_node->var->size > 1)
		_x64_clear_var(ctx, dst->dag_node, _3ac->instructions);

	_x64_op_2(ctx, SCF_X64_CMP, src1->dag_node, src0->dag_node, _3ac->instructions);
	_x64_cmp_set(ctx, SCF_X64_SETL, dst->dag_node, _3ac->instructions);
	return 0;
}

static int _x64_3ac_assign_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	scf_x64_context_t* x64 = ctx->priv;

	assert(_3ac->dst && _3ac->dst->node);
	assert(1 == _3ac->srcs->size);

	scf_3ac_operand_t* dst = _3ac->dst;
	scf_3ac_operand_t* src = _3ac->srcs->data[0];
	assert(src && src->dag_node);
	assert(dst && dst->dag_node);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	_x64_op_2(ctx, SCF_X64_MOV, dst->dag_node, src->dag_node, _3ac->instructions);

	return 0;
}

static int _x64_3ac_return_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	assert(1 == _3ac->srcs->size);

	scf_x64_context_t* x64 = ctx->priv;

	scf_3ac_operand_t* src = _3ac->srcs->data[0];
	assert(src && src->dag_node);

	if (!_3ac->instructions) {
		_3ac->instructions = scf_vector_alloc();
	}

	scf_register_x64_t*	rd	   = NULL;
	scf_register_x64_t*	rs	   = NULL; 
	scf_x64_OpCode_t*   OpCode = NULL;
	scf_instruction_t*  inst   = NULL;
	scf_variable_t*		var    = src->dag_node->var;

	if (8 == var->size)
		rd = (scf_register_x64_t*)_x64_find_register("rax");
	else
		rd = (scf_register_x64_t*)_x64_find_register("eax");

	if (src->dag_node->color > 0) {
		OpCode = _x64_find_OpCode(SCF_X64_MOV, var->size, var->size, SCF_X64_G2E);
		assert(OpCode);
		rs   = _x64_register_get_by_color(x64, src->dag_node->color, var->size);
		inst = _x64_make_instruction_G2G(OpCode, rd, rs);
	} else if (src->dag_node->color < 0) {
		OpCode = _x64_find_OpCode(SCF_X64_MOV, var->size, var->size, SCF_X64_E2G);
		assert(OpCode);
		inst = _x64_make_instruction_E2G(OpCode, rd, var);
	} else {
		printf("%s(),%d, dag_node: %p, var: %s\n", __func__, __LINE__, src->dag_node, var->w->text->data);
		assert(var->const_literal_flag);

		OpCode = _x64_find_OpCode(SCF_X64_MOV, var->size, var->size, SCF_X64_I2G);
		assert(OpCode);
		inst = _x64_make_instruction_I2G(OpCode, rd, var);
	}

	scf_vector_add(_3ac->instructions, inst);
#if 0
	OpCode = _x64_find_OpCode(SCF_X64_RET, 8, 8, SCF_X64_G);
	assert(OpCode);
	inst = _x64_make_instruction_G(OpCode, rd);
	scf_vector_add(_3ac->instructions, inst);
#endif
	return 0;
}

static int _x64_3ac_goto_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	return 0;
}

static int _x64_3ac_jz_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	//printf("%s(),%d, error\n", __func__, __LINE__);
	return 0;
}

static int _x64_3ac_jnz_handler(scf_native_context_t* ctx, scf_3ac_code_t* _3ac)
{
	return 0;
}

static scf_x64_3ac_handler_t	_x64_3ac_handlers[] = {

	{SCF_OP_CALL,			_x64_3ac_call_handler},
	{SCF_OP_ARRAY_INDEX, 	_x64_3ac_array_index_handler},

	{SCF_OP_LOGIC_NOT, 		_x64_3ac_logic_not_handler},
	{SCF_OP_NEG, 			_x64_3ac_neg_handler},
	{SCF_OP_POSITIVE, 		_x64_3ac_positive_handler},

	{SCF_OP_DEREFERENCE, 	_x64_3ac_dereference_handler},
	{SCF_OP_ADDRESS_OF, 	_x64_3ac_address_of_handler},

	{SCF_OP_MUL, 			_x64_3ac_mul_handler},
	{SCF_OP_DIV, 			_x64_3ac_div_handler},
	{SCF_OP_ADD, 			_x64_3ac_add_handler},
	{SCF_OP_SUB, 			_x64_3ac_sub_handler},

	{SCF_OP_EQ, 			_x64_3ac_eq_handler},
	{SCF_OP_GT, 			_x64_3ac_gt_handler},
	{SCF_OP_LT, 			_x64_3ac_lt_handler},

	{SCF_OP_ASSIGN, 		_x64_3ac_assign_handler},

	{SCF_OP_RETURN, 		_x64_3ac_return_handler},
	{SCF_OP_GOTO, 			_x64_3ac_goto_handler},

	{SCF_OP_3AC_JE, 		_x64_3ac_jz_handler},
	{SCF_OP_3AC_JNE, 		_x64_3ac_jnz_handler},
};

scf_x64_3ac_handler_t*	scf_x64_find_3ac_handler(const int _3ac_op_type)
{
	int i;
	for (i = 0; i < sizeof(_x64_3ac_handlers) / sizeof(_x64_3ac_handlers[0]); i++) {

		scf_x64_3ac_handler_t* handler = &(_x64_3ac_handlers[i]);

		if (_3ac_op_type == handler->type)
			return handler;
	}
	return NULL;
}
