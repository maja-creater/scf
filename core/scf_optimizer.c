#include"scf_optimizer.h"

extern scf_optimizer_t   scf_optimizer_dag;

extern scf_optimizer_t   scf_optimizer_call;
extern scf_optimizer_t   scf_optimizer_pointer_alias;
extern scf_optimizer_t   scf_optimizer_active_vars;

extern scf_optimizer_t   scf_optimizer_pointer_aliases;
extern scf_optimizer_t   scf_optimizer_loads_saves;

extern scf_optimizer_t   scf_optimizer_dominators_normal;
extern scf_optimizer_t   scf_optimizer_dominators_reverse;

extern scf_optimizer_t   scf_optimizer_auto_gc_find;
extern scf_optimizer_t   scf_optimizer_auto_gc;

extern scf_optimizer_t   scf_optimizer_basic_block;
extern scf_optimizer_t   scf_optimizer_loop;
extern scf_optimizer_t   scf_optimizer_group;

extern scf_optimizer_t   scf_optimizer_generate_loads_saves;

static scf_optimizer_t*  scf_optimizers_local0[] =
{
	&scf_optimizer_dag,

	&scf_optimizer_call,
	&scf_optimizer_pointer_alias,
	&scf_optimizer_active_vars,

	&scf_optimizer_pointer_aliases,
	&scf_optimizer_loads_saves,

	&scf_optimizer_dominators_normal,
};

static scf_optimizer_t*  scf_optimizers_global0[] =
{
	&scf_optimizer_auto_gc_find,
};

static scf_optimizer_t*  scf_optimizers_local1[] =
{
	&scf_optimizer_auto_gc,

	&scf_optimizer_basic_block,

	&scf_optimizer_dominators_normal,
	&scf_optimizer_loop,

	&scf_optimizer_group,

	&scf_optimizer_generate_loads_saves,
};

static void __scf_loops_print(scf_bb_group_t* loop)
{
	scf_basic_block_t* bb;

	int j;
	int k;

	if (loop->loop_childs) {

		for (k = 0; k < loop->loop_childs->size; k++)
			__scf_loops_print(loop->loop_childs->data[k]);
	}

	printf("\033[33mloop:  %p\033[0m\n", loop);

	printf("\033[34mentry:\033[0m\n");
	for (j = 0; j < loop->entries->size; j++) {
		bb =        loop->entries->data[j];

		printf("%p, %d\n", bb, bb->index);
	}

	printf("\033[35mexit:\033[0m\n");
	for (j = 0; j < loop->exits->size; j++) {
		bb =        loop->exits->data[j];

		printf("%p, %d\n", bb, bb->index);
	}

	printf("\033[36mbody:\033[0m\n");
	for (j = 0; j < loop->body->size; j++) {
		bb =        loop->body->data[j];

		printf("%p, %d\n", bb, bb->index);
	}

	if (loop->loop_childs) {
		printf("childs: %d\n", loop->loop_childs->size);
		for (k = 0; k <   loop->loop_childs->size; k++)
			printf("%p ", loop->loop_childs->data[k]);
		printf("\n");
	}
	if (loop->loop_parent)
		printf("parent: %p\n", loop->loop_parent);
	printf("loop_layers: %d\n\n", loop->loop_layers);
}

void scf_loops_print(scf_vector_t* loops)
{
	int i;
	for (i = 0; i < loops->size; i++)
		__scf_loops_print(loops->data[i]);
}

void scf_groups_print(scf_vector_t* groups)
{
	int i;
	for (i = 0; i < groups->size; i++)
		scf_bb_group_print(groups->data[i]);
}

static int __scf_optimize_local(scf_ast_t* ast, scf_vector_t* functions, scf_optimizer_t** optimizers, int nb_optimizers)
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

			int ret = opt->optimize(ast, f, &f->basic_block_list_head, NULL);
			if (ret < 0) {
				scf_loge("optimizer: %s\n", opt->name);
				return ret;
			}
		}
	}

	return 0;
}

static int __scf_optimize_global(scf_ast_t* ast, scf_vector_t* functions, scf_optimizer_t** optimizers, int nb_optimizers)
{
	scf_optimizer_t* opt;

	int j;
	for (j  = 0; j < nb_optimizers; j++) {

		opt = optimizers[j];

		if (!opt)
			continue;

		int ret = opt->optimize(ast, NULL, NULL, functions);
		if (ret < 0) {
			scf_loge("optimizer: %s\n", opt->name);
			return ret;
		}
	}

	return 0;
}

int scf_optimize(scf_ast_t* ast, scf_vector_t* functions)
{
	int nb_local0  = sizeof(scf_optimizers_local0)  / sizeof(scf_optimizers_local0[0]);
	int nb_local1  = sizeof(scf_optimizers_local1)  / sizeof(scf_optimizers_local1[0]);
	int nb_global0 = sizeof(scf_optimizers_global0) / sizeof(scf_optimizers_global0[0]);

	int ret = __scf_optimize_local(ast, functions, scf_optimizers_local0, nb_local0);
	if (ret < 0)
		return ret;
#if 1
	ret = __scf_optimize_global(ast, functions, scf_optimizers_global0, nb_global0);
	if (ret < 0)
		return ret;
#endif
	ret = __scf_optimize_local(ast, functions, scf_optimizers_local1, nb_local1);
	if (ret < 0)
		return ret;

#if 1
	int i;
	for (i = 0; i < functions->size; i++) {

		scf_function_t* f = functions->data[i];

		if (!f->node.define_flag)
			continue;

		scf_basic_block_print_list(&f->basic_block_list_head);
		scf_loops_print(f->bb_loops);
		scf_groups_print(f->bb_groups);
	}
#endif
	return 0;
}

