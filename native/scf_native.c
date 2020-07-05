#include"scf_native.h"

extern scf_native_ops_t native_ops_x64;

scf_native_ops_t*	native_ops_array[] = {
	&native_ops_x64,

	NULL,
};

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

int scf_native_write_elf(scf_native_t* ctx, const char* path, scf_function_t* f)
{
	if (ctx && path && f) {
		if (ctx->ops && ctx->ops->write_elf)
			return ctx->ops->write_elf(ctx, path);
	}

	printf("%s(),%d, error: \n", __func__, __LINE__);
	return -1;
}

