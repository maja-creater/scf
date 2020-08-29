#include"scf_optimizer.h"

extern scf_optimizer_t   scf_optimizer_active_vars;

extern scf_optimizer_t   scf_optimizer_loop;

extern scf_optimizer_t   scf_optimizer_generate_loads_saves;


static scf_optimizer_t*  scf_optimizers[] =
{
	&scf_optimizer_active_vars,

	&scf_optimizer_loop,

	&scf_optimizer_generate_loads_saves,
};


int scf_optimize(scf_function_t* f, scf_list_t* bb_list_head)
{
	int i;

	for (i = 0; i < sizeof(scf_optimizers) / sizeof(scf_optimizers[0]); i++) {

		scf_optimizer_t* optimizer = scf_optimizers[i];

		if (!optimizer)
			continue;

		int ret = optimizer->optimize(f, bb_list_head);
		if (ret < 0)
			return ret;
	}

	return 0;
}

