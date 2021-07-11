#include"scf_x64.h"
#include"scf_elf.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

static void _x64_peephole_del_push_by_pop(scf_vector_t* insts, scf_instruction_t* pop)
{
	scf_instruction_t* push = NULL;
	int k;

	for (k = pop->c->instructions->size - 1; k >= 0; k--) {

		if (pop->c->instructions->data[k] == pop)
			break;
	}

	for (--k; k >= 0; k--) {
		push = pop->c->instructions->data[k];

		if (scf_inst_data_same(&push->src, &pop->dst))
			break;
	}
	assert(k >= 0);

	assert(0 == scf_vector_del(pop->c->instructions, push));

	scf_vector_del(insts, push);

	scf_loge("del: \n");
	scf_instruction_print(push);

	free(push);
	push = NULL;
}

static int _x64_peephole_common(scf_vector_t* std_insts, scf_instruction_t* inst)
{
	scf_3ac_code_t*    c  = inst->c;
	scf_basic_block_t* bb = c->basic_block;

	scf_instruction_t* inst2;
	scf_instruction_t* std;

	int j;
	for (j  = std_insts->size - 1; j >= 0; j--) {
		std = std_insts->data[j];
#if 0
		scf_loge("std j: %d\n", j);
		scf_3ac_code_print(std->c, NULL);
		scf_instruction_print(std);

		scf_loge("inst: \n");
		scf_instruction_print(inst);
		printf("\n");
#endif
		if (scf_inst_data_same(&std->dst, &inst->dst)) {

			if (scf_inst_data_same(&std->src, &inst->src)) {

				assert(0 == scf_vector_del(inst->c->instructions, inst));

				free(inst);
				inst = NULL;
				return X64_PEEPHOLE_DEL;
			}

			assert(0 == scf_vector_del(std_insts, std));

			if (std->nb_used > 0)
				continue;

			if (SCF_X64_POP == std->OpCode->type)
				_x64_peephole_del_push_by_pop(std_insts, std);

			assert(0 == scf_vector_del(std->c->instructions, std));

			free(std);
			std = NULL;
			continue;
		} else if (scf_inst_data_same(&std->src, &inst->src)) {
			continue;
		} else if (scf_inst_data_same(&std->dst, &inst->src)) {

			std->nb_used++;

			if (scf_inst_data_same(&std->src, &inst->dst)) {

				assert(0 == scf_vector_del(inst->c->instructions, inst));

				free(inst);
				inst = NULL;
				return X64_PEEPHOLE_DEL;
			}

			if (inst->src.flag) {
				assert(std->dst.flag);

				inst2 = x64_make_inst_E2G((scf_x64_OpCode_t*)inst->OpCode,
						(scf_register_x64_t*)inst->dst.base,
						(scf_register_x64_t*)std->src.base);
				if (!inst2)
					return -ENOMEM;

				memcpy(inst->code, inst2->code, inst2->len);
				inst->len = inst2->len;

				inst->src.base  = std->src.base;
				inst->src.index = NULL;
				inst->src.scale = 0;
				inst->src.disp  = 0;
				inst->src.flag  = 0;

				free(inst2);
				inst2 = NULL;
				continue;

			} else if (inst->dst.flag) {

				if (x64_inst_data_is_reg(&inst->src)
						&& x64_inst_data_is_reg(&std->src)
						&& inst->src.base != std->src.base) {

					if (!inst->dst.index)
						inst2 = x64_make_inst_G2P((scf_x64_OpCode_t*)inst->OpCode,
								(scf_register_x64_t*)inst->dst.base, inst->dst.disp,
								(scf_register_x64_t*)std->src.base);
					else
						inst2 = x64_make_inst_G2SIB((scf_x64_OpCode_t*)inst->OpCode,
								(scf_register_x64_t*)inst->dst.base,
								(scf_register_x64_t*)inst->dst.index, inst->dst.scale, inst->dst.disp,
								(scf_register_x64_t*)std->src.base);
					if (!inst2)
						return -ENOMEM;

					memcpy(inst->code, inst2->code, inst2->len);
					inst->len = inst2->len;

					inst->src.base  = std->src.base;
					inst->src.index = NULL;
					inst->src.scale = 0;
					inst->src.disp  = 0;
					inst->src.flag  = 0;

					free(inst2);
					inst2 = NULL;
					continue;
				}
			}
		} else if (scf_inst_data_same(&std->src, &inst->dst)) {

			assert(0 == scf_vector_del(std_insts, std));

		} else if (x64_inst_data_is_reg(&std->src)) {

			scf_register_x64_t* r0;
			scf_register_x64_t* r1;

			if (x64_inst_data_is_reg(&inst->dst)) {

				r0 = (scf_register_x64_t*) std ->src.base;
				r1 = (scf_register_x64_t*) inst->dst.base;

				if (X64_COLOR_CONFLICT(r0->color, r1->color))
					assert(0 == scf_vector_del(std_insts, std));
			}

		} else if (x64_inst_data_is_reg(&std->dst)) {

			if (inst->src.base == std->dst.base
					|| inst->src.index == std->dst.base
					|| inst->dst.base  == std->dst.base
					|| inst->dst.index == std->dst.base)
				std->nb_used++;
		}
	}

	assert(0 == scf_vector_add_unique(std_insts, inst));
	return 0;
}

static int _x64_peephole_cmp(scf_vector_t* std_insts, scf_instruction_t* inst)
{
	if (0 == inst->src.flag && 0 == inst->dst.flag)
		return 0;

	scf_3ac_code_t*    c  = inst->c;
	scf_basic_block_t* bb = c->basic_block;

	scf_instruction_t* inst2;
	scf_instruction_t* std;

	int j;
	for (j  = std_insts->size - 1; j >= 0; j--) {
		std = std_insts->data[j];

		if (inst->src.flag) {

			if (scf_inst_data_same(&inst->src, &std->src))

				inst->src.base = std->dst.base;

			else if (scf_inst_data_same(&inst->src, &std->dst))

				inst->src.base = std->src.base;
			else
				goto check;

			inst2 = x64_make_inst_E2G((scf_x64_OpCode_t*) inst->OpCode,
					(scf_register_x64_t*) inst->dst.base,
					(scf_register_x64_t*) inst->src.base);
			if (!inst2)
				return -ENOMEM;

			inst->src.index = NULL;
			inst->src.scale = 0;
			inst->src.disp  = 0;
			inst->src.flag  = 0;

		} else if (inst->dst.flag) {

			if (scf_inst_data_same(&inst->dst, &std->src))

				inst->dst.base  = std->dst.base;

			else if (scf_inst_data_same(&inst->dst, &std->dst))

				inst->dst.base  = std->src.base;
			else
				goto check;

			if (inst->src.imm_size > 0)
				inst2 = x64_make_inst_I2E((scf_x64_OpCode_t*)inst->OpCode,
						(scf_register_x64_t*)inst->dst.base,
						(uint8_t*)&inst->src.imm, inst->src.imm_size);
			else
				inst2 = x64_make_inst_G2E((scf_x64_OpCode_t*)inst->OpCode,
						(scf_register_x64_t*)inst->dst.base,
						(scf_register_x64_t*)inst->src.base);
			if (!inst2)
				return -ENOMEM;

			inst->dst.index = NULL;
			inst->dst.scale = 0;
			inst->dst.disp  = 0;
			inst->dst.flag  = 0;
		} else
			goto check;

		memcpy(inst->code, inst2->code, inst2->len);
		inst->len = inst2->len;

		free(inst2);
		inst2 = NULL;

check:
		if (x64_inst_data_is_reg(&std->dst)) {

			if (inst->src.base == std->dst.base
					|| inst->src.index == std->dst.base
					|| inst->dst.index == std->dst.base
					|| inst->dst.base  == std->dst.base)
				std->nb_used++;
		}
	}
	return 0;
}

static void _x64_peephole_function(scf_vector_t* tmp_insts, scf_function_t* f, int jmp_back_flag)
{
	scf_register_x64_t* rax = x64_find_register("rax");
	scf_register_x64_t* rsp = x64_find_register("rsp");
	scf_register_x64_t* rbp = x64_find_register("rbp");

	scf_register_x64_t* rdi = x64_find_register("rdi");
	scf_register_x64_t* rsi = x64_find_register("rsi");
	scf_register_x64_t* rdx = x64_find_register("rdx");
	scf_register_x64_t* rcx = x64_find_register("rcx");
	scf_register_x64_t* r8  = x64_find_register("r8");
	scf_register_x64_t* r9  = x64_find_register("r9");

	scf_instruction_t*  inst;
	scf_instruction_t*  inst2;
	scf_basic_block_t*  bb;
	scf_3ac_code_t*     c;
	scf_list_t*         l;

	int i;
	int j;

	for (i   = tmp_insts->size - 1; i >= 0; i--) {
		inst = tmp_insts->data[i];

		scf_register_x64_t* r0;
		scf_register_x64_t* r1;

		if (SCF_X64_PUSH == inst->OpCode->type)
			continue;

		else if (SCF_X64_POP == inst->OpCode->type) {

			r0 = (scf_register_x64_t*)inst->dst.base;

			if (x64_inst_data_is_reg(&inst->dst)
					&& (r0 == rax || r0 == rsp || r0 == rbp))
				continue;

		} else if (SCF_X64_MOV != inst->OpCode->type)
			continue;

		if (x64_inst_data_is_reg(&inst->dst)) {

			r0 = (scf_register_x64_t*)inst->dst.base;

			if (X64_COLOR_CONFLICT(rax->color, r0->color))
				continue;

		} else if (!x64_inst_data_is_local(&inst->dst))
			continue;

		if (jmp_back_flag)
			j = 0;
		else
			j = i + 1;

		for ( ; j < tmp_insts->size; j++) {
			inst2 = tmp_insts->data[j];

			if (inst == inst2)
				continue;

			if (scf_inst_data_same(&inst->dst, &inst2->src))
				break;

			if (x64_inst_data_is_reg(&inst->dst)) {

				r0 = (scf_register_x64_t*)inst ->dst.base;
				r1 = (scf_register_x64_t*)inst2->src.base;

				if (SCF_X64_CALL == inst2->OpCode->type) {

					if (X64_COLOR_CONFLICT(r0->color, rdi->color)
							|| X64_COLOR_CONFLICT(r0->color, rsi->color)
							|| X64_COLOR_CONFLICT(r0->color, rdx->color)
							|| X64_COLOR_CONFLICT(r0->color, rcx->color))
						break;
				} else {
					if (x64_inst_data_is_reg(&inst2->src)) {
						if (X64_COLOR_CONFLICT(r0->color, r1->color))
							break;
					}

					if (inst2->src.base  == inst->dst.base
							|| inst2->src.index == inst->dst.base
							|| inst2->dst.index == inst->dst.base
							|| inst2->dst.base  == inst->dst.base)
						break;
				}

			} else if (x64_inst_data_is_local(&inst->dst)) {

				if (scf_inst_data_same(&inst->dst, &inst2->src))
					break;
				else if (rsp == (scf_register_x64_t*)inst->dst.base)
					break;
				else if (SCF_OP_VA_START == inst->c->op->type
						|| SCF_OP_VA_ARG == inst->c->op->type
						|| SCF_OP_VA_END == inst->c->op->type)
					break;

				else if (SCF_X64_CMP    == inst2->OpCode->type
						|| SCF_X64_TEST == inst2->OpCode->type) {

					if (scf_inst_data_same(&inst->dst, &inst2->dst))
						break;

				} else if (SCF_X64_LEA  == inst2->OpCode->type) {
					if (inst2->src.base == inst->dst.base)
						break;
				}
			}
		}

		if (j < tmp_insts->size)
			continue;

		c  = inst->c;

		if (SCF_X64_POP == inst->OpCode->type) {
			_x64_peephole_del_push_by_pop(tmp_insts, inst);
			i--;
		}
		assert(0 == scf_vector_del(c->instructions,  inst));
		assert(0 == scf_vector_del(tmp_insts,        inst));

		scf_loge("del: \n");
		scf_instruction_print(inst);

		free(inst);
		inst = NULL;
	}

	int nb_locals = 0;

	for (i = 0; i < tmp_insts->size; i++) {
		inst      = tmp_insts->data[i];

		if (x64_inst_data_is_local(&inst->src)
				|| x64_inst_data_is_local(&inst->dst))
			nb_locals++;
	}

	if (nb_locals > 0)
		f->bp_used_flag = 1;
	else {
		f->bp_used_flag = 0;

		l  = scf_list_tail(&f->basic_block_list_head);
		bb = scf_list_data(l, scf_basic_block_t, list);

		l = scf_list_tail(&bb->code_list_head);
		c = scf_list_data(l, scf_3ac_code_t, list);

		assert(SCF_OP_3AC_END == c->op->type);

		for (i   = c->instructions->size - 2; i >= 0; i--) {
			inst = c->instructions->data[i];

			c ->inst_bytes -= inst->len;
			bb->code_bytes -= inst->len;

			assert(0 == scf_vector_del(c->instructions, inst));

			free(inst);
			inst = NULL;
		}

		assert(1 == c->instructions->size);

		inst = c->instructions->data[0];
		assert(SCF_X64_RET == inst->OpCode->type);
	}
}

int x64_optimize_peephole(scf_native_t* ctx, scf_function_t* f)
{
	scf_register_x64_t* rbp = x64_find_register("rbp");
	scf_instruction_t*  std;
	scf_instruction_t*  inst;
	scf_instruction_t*  inst2;
	scf_basic_block_t*  bb;
	scf_3ac_operand_t*  dst;
	scf_3ac_code_t*     c;
	scf_list_t*         l;
	scf_list_t*         l2;

	scf_vector_t*       std_insts;
	scf_vector_t*       tmp_insts; // instructions for register or local variable

	std_insts = scf_vector_alloc();
	if (!std_insts)
		return -ENOMEM;

	tmp_insts = scf_vector_alloc();
	if (!tmp_insts) {
		scf_vector_free(tmp_insts);
		return -ENOMEM;
	}

	int jmp_back_flag = 0;

	int ret = 0;

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		if (bb->jmp_flag) {
			scf_vector_clear(std_insts, NULL);

			l2 = scf_list_head(&bb->code_list_head);
			c  = scf_list_data(l2, scf_3ac_code_t, list);

			assert(1 == c->dsts->size);
			dst = c->dsts->data[0];

			if (dst->bb->index < bb->index)
				jmp_back_flag = 1;

			continue;
		}

		if (bb->jmp_dst_flag)
			scf_vector_clear(std_insts, NULL);

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head);
				l2 = scf_list_next(l2)) {

			c = scf_list_data(l2, scf_3ac_code_t, list);

			if (!c->instructions)
				continue;

			int i;
			for (i = 0; i < c->instructions->size; ) {
				inst      = c->instructions->data[i];

				assert(inst->OpCode);

				inst->c = c;

				if (SCF_X64_MOV != inst->OpCode->type
						&& SCF_X64_CMP  != inst->OpCode->type
						&& SCF_X64_TEST != inst->OpCode->type
						&& SCF_X64_PUSH != inst->OpCode->type
						&& SCF_X64_POP  != inst->OpCode->type) {
					scf_vector_clear(std_insts, NULL);
					goto next;
				}

				if (SCF_X64_PUSH == inst->OpCode->type)
					goto next;

				if (SCF_X64_CMP == inst->OpCode->type || SCF_X64_TEST == inst->OpCode->type) {

					ret = _x64_peephole_cmp(std_insts, inst);
					if (ret < 0) {
						scf_loge("\n");
						goto error;
					}

					goto next;
				}

				ret = _x64_peephole_common(std_insts, inst);
				if (ret < 0)
					goto error;

				if (X64_PEEPHOLE_DEL == ret)
					continue;

next:
				ret = scf_vector_add(tmp_insts, inst);
				if (ret < 0)
					goto error;
				i++;
			}
		}
	}

	_x64_peephole_function(tmp_insts, f, jmp_back_flag);

	ret = 0;
error:
	scf_vector_free(tmp_insts);
	scf_vector_free(std_insts);
	return ret;
}

