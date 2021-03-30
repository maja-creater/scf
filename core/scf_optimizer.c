#include"scf_optimizer.h"

extern scf_optimizer_t   scf_optimizer_dag;

extern scf_optimizer_t   scf_optimizer_call;
extern scf_optimizer_t   scf_optimizer_pointer_alias;

extern scf_optimizer_t   scf_optimizer_auto_gc;

extern scf_optimizer_t   scf_optimizer_active_vars;
extern scf_optimizer_t   scf_optimizer_pointer_aliases;
extern scf_optimizer_t   scf_optimizer_loads_saves;

extern scf_optimizer_t   scf_optimizer_basic_block;
extern scf_optimizer_t   scf_optimizer_loop;

extern scf_optimizer_t   scf_optimizer_generate_loads_saves;

static scf_optimizer_t*  scf_optimizers[] =
{
	&scf_optimizer_dag,
#if 1
	&scf_optimizer_call,
	&scf_optimizer_pointer_alias,

	&scf_optimizer_auto_gc,

	&scf_optimizer_active_vars,
	&scf_optimizer_pointer_aliases,
	&scf_optimizer_loads_saves,

	&scf_optimizer_basic_block,

	&scf_optimizer_loop,

	&scf_optimizer_generate_loads_saves,
#endif
};


int scf_optimize(scf_function_t* f, scf_list_t* bb_list_head)
{
	int i;

	for (i = 0; i < sizeof(scf_optimizers) / sizeof(scf_optimizers[0]); i++) {

		scf_optimizer_t* optimizer = scf_optimizers[i];

		if (!optimizer)
			continue;

		int ret = optimizer->optimize(f, bb_list_head);
		if (ret < 0) {
			scf_loge("optimizer: %s\n", optimizer->name);
			return ret;
		}
	}

	return 0;
}

