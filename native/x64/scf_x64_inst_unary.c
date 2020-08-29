#include"scf_x64.h"

static int _unary_assign_sib(x64_sib_t* sib, int size, scf_3ac_code_t* c, scf_function_t* f, int OpCode_type)
{
	scf_x64_OpCode_t*   OpCode;
	scf_instruction_t*  inst;

	OpCode = x64_find_OpCode(OpCode_type, size, size, SCF_X64_E);
	if (!OpCode) {
		scf_loge("\n");
		return -EINVAL;
	}

	if (sib->index)
		inst = x64_make_inst_SIB(OpCode, sib->base, sib->index, sib->scale, sib->disp, size);
	else
		inst = x64_make_inst_P(OpCode, sib->base, sib->disp, size);
	X64_INST_ADD_CHECK(c->instructions, inst);
	return 0;
}

int x64_unary_assign_dereference(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 1) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_x64_context_t*  x64   = ctx->priv;
	scf_function_t*     f     = x64->f;
	scf_3ac_operand_t*  base  = c->srcs->data[0];

	if (!base || !base->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	x64_sib_t sib = {0};

	int size = base->dag_node->var->data_size;

	int ret  = x64_dereference_reg(&sib, base->dag_node, NULL, c, f);
	if (ret < 0)
		return ret;

	return _unary_assign_sib(&sib, size, c, f, OpCode_type);
}

int x64_unary_assign_pointer(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 2) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_x64_context_t*  x64    = ctx->priv;
	scf_function_t*     f      = x64->f;

	scf_3ac_operand_t*  base   = c->srcs->data[0];
	scf_3ac_operand_t*  member = c->srcs->data[1];

	if (!base || !base->dag_node)
		return -EINVAL;

	if (!member || !member->dag_node)
		return -EINVAL;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	x64_sib_t sib = {0};

	int size = member->dag_node->var->data_size;

	int ret  = x64_pointer_reg(&sib, base->dag_node, member->dag_node, c, f);
	if (ret < 0)
		return ret;

	return _unary_assign_sib(&sib, size, c, f, OpCode_type);
}

int x64_unary_assign_array_index(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type)
{
	if (!c->srcs || c->srcs->size != 3) {
		scf_loge("\n");
		return -EINVAL;
	}

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

	x64_sib_t sib = {0};

	int size = base->dag_node->var->data_size;

	int ret  = x64_array_index_reg(&sib, base->dag_node, index->dag_node, scale->dag_node, c, f);
	if (ret < 0)
		return ret;

	return _unary_assign_sib(&sib, size, c, f, OpCode_type);
}

