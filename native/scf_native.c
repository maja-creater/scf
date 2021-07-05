#include"scf_native.h"

extern scf_native_ops_t native_ops_x64;

scf_native_ops_t*	native_ops_array[] = {
	&native_ops_x64,

	NULL,
};

void scf_instruction_print(scf_instruction_t* inst)
{
	if (inst->OpCode)
		printf("%s ", inst->OpCode->name);

	if (1 == inst->src.flag) {
		if (inst->src.index)
			printf("%d(%s, %s, %d), ", inst->src.disp, inst->src.base->name,
					inst->src.index->name, inst->src.scale);

		else if (inst->src.base)
			printf("%d(%s), ", inst->src.disp, inst->src.base->name);
		else
			printf("%d(rip), ", inst->dst.disp);

	} else if (inst->src.base)
		printf("%s, ", inst->src.base->name);

	else if (inst->src.imm_size > 0)
		printf("%d, ", (int)inst->src.imm);

	if (1 == inst->dst.flag) {
		if (inst->dst.index)
			printf("%d(%s, %s, %d), ", inst->dst.disp, inst->dst.base->name,
					inst->dst.index->name, inst->dst.scale);

		else if (inst->dst.base)
			printf("%d(%s), ", inst->dst.disp, inst->dst.base->name);
		else
			printf("%d(rip), ", inst->dst.disp);

	} else if (inst->dst.base)
		printf("%s, ", inst->dst.base->name);

	else if (inst->dst.imm_size > 0)
		printf("%d, ", (int)inst->dst.imm);

	printf("\n");
}

int scf_native_open(scf_native_t** pctx, const char* name)
{
	scf_native_t* ctx = calloc(1, sizeof(scf_native_t));
	assert(ctx);

	int i;
	for (i = 0; native_ops_array[i]; i++) {
		if (!strcmp(native_ops_array[i]->name, name)) {
			ctx->ops = native_ops_array[i];
			break;
		}
	}

	if (!ctx->ops) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	if (ctx->ops->open && ctx->ops->open(ctx) == 0) {
		*pctx = ctx;
		return 0;
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);

	free(ctx);
	ctx = NULL;
	return -1;
}

int scf_native_close(scf_native_t* ctx)
{
	if (ctx) {
		if (ctx->ops && ctx->ops->close) {
			ctx->ops->close(ctx);
		}

		free(ctx);
		ctx = NULL;
	}
	return 0;
}

int scf_native_select_inst(scf_native_t* ctx, scf_function_t* f)
{
	if (ctx && f) {
		if (ctx->ops && ctx->ops->select_inst)
			return ctx->ops->select_inst(ctx, f);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

