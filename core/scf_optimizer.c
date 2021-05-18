#include"scf_optimizer.h"

extern scf_optimizer_t   scf_optimizer_dag;

extern scf_optimizer_t   scf_optimizer_call;
extern scf_optimizer_t   scf_optimizer_pointer_alias;
extern scf_optimizer_t   scf_optimizer_active_vars;

extern scf_optimizer_t   scf_optimizer_pointer_aliases;
extern scf_optimizer_t   scf_optimizer_loads_saves;

extern scf_optimizer_t   scf_optimizer_dominators;

extern scf_optimizer_t   scf_optimizer_auto_gc;

extern scf_optimizer_t   scf_optimizer_basic_block;
extern scf_optimizer_t   scf_optimizer_loop;

extern scf_optimizer_t   scf_optimizer_generate_loads_saves;

static scf_optimizer_t*  scf_optimizers_0[] =
{
	&scf_optimizer_dag,
#if 1
	&scf_optimizer_call,
	&scf_optimizer_pointer_alias,
	&scf_optimizer_active_vars,

	&scf_optimizer_pointer_aliases,
	&scf_optimizer_loads_saves,

	&scf_optimizer_dominators,
#endif
};

static scf_optimizer_t*  scf_optimizers_1[] =
{
#if 1
	&scf_optimizer_basic_block,

	&scf_optimizer_dominators,
	&scf_optimizer_loop,

	&scf_optimizer_generate_loads_saves,
#endif
};

static void _scf_loops_print(scf_vector_t* loops)
{
	int i;
	int j;
	int k;

	for (i = 0; i < loops->size; i++) {
		scf_bb_group_t* loop = loops->data[i];

		printf("loop:  %p\n", loop);
		printf("entry: %p\n", loop->entry);
		printf("exit:  %p\n", loop->exit);
		printf("body: ");
		for (j = 0; j < loop->body->size; j++)
			printf("%p ", loop->body->data[j]);
		printf("\n");

		if (loop->loop_childs) {
			printf("childs: ");
			for (k = 0; k < loop->loop_childs->size; k++)
				printf("%p ", loop->loop_childs->data[k]);
			printf("\n");
		}
		if (loop->loop_parent)
			printf("parent: %p\n", loop->loop_parent);
		printf("loop_layers: %d\n\n", loop->loop_layers);
	}
}

static int __scf_optimize(scf_ast_t* ast, scf_vector_t* functions, scf_optimizer_t** optimizers, int nb_optimizers)
{
	scf_optimizer_t* opt;
	scf_function_t*  f;

	int i;
	for (i = 0; i < functions->size; i++) {
		f  =        functions->data[i];

		if (!f->node.define_flag)
			continue;

		int j;
		for (j  = 0; j < nb_optimizers; j++) {

			opt = optimizers[j];

			if (!opt)
				continue;

			int ret = opt->optimize(ast, f, &f->basic_block_list_head);
			if (ret < 0) {
				scf_loge("optimizer: %s\n", opt->name);
				return ret;
			}
		}

#if 1
		scf_basic_block_print_list(&f->basic_block_list_head);
		_scf_loops_print(f->bb_loops);
#endif
	}

	return 0;
}

int scf_optimize(scf_ast_t* ast, scf_vector_t* functions)
{
	int nb_optimizers0 = sizeof(scf_optimizers_0) / sizeof(scf_optimizers_0[0]);
	int nb_optimizers1 = sizeof(scf_optimizers_1) / sizeof(scf_optimizers_1[0]);

	int ret = __scf_optimize(ast, functions, scf_optimizers_0, nb_optimizers0);
	if (ret < 0)
		return ret;

	ret = __scf_optimize(ast, functions, scf_optimizers_1, nb_optimizers1);
	if (ret < 0)
		return ret;

	return 0;
}

