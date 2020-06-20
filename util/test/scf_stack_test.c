#include"scf_stack.h"

int main()
{
	scf_stack_t* s = scf_stack_alloc();

	int i;
	for (i = 0; i < 16; i++) {
		scf_stack_push(s, (void*)i);
	}

	for (i = 0; i < 16; i++) {
		int j = (int)scf_stack_pop(s);
		printf("i: %d, j: %d, s->size: %d, s->capacity: %d\n", i, j, s->size, s->capacity);
	}

	int j = (int)scf_stack_pop(s);
	printf("i: %d, j: %d, s->size: %d, s->capacity: %d\n", i, j, s->size, s->capacity);
}
