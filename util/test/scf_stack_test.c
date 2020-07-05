#include"../scf_stack.h"

int main()
{
	scf_stack_t* s = scf_stack_alloc();

	int i;
	for (i = 0; i < 64; i++) {
		printf("i: %d, s->size: %d, s->capacity: %d\n", i, s->size, s->capacity);
		scf_stack_push(s, (void*)i);
	}

	for (i = 0; i < 64; i++) {
		int j = (int)scf_stack_pop(s);
		printf("i: %d, j: %d, s->size: %d, s->capacity: %d\n", i, j, s->size, s->capacity);
	}

	int j = (int)scf_stack_pop(s);
	printf("i: %d, j: %d, s->size: %d, s->capacity: %d\n", i, j, s->size, s->capacity);
}
