#include"scf_object.h"

#define __sys_malloc  malloc
#define __sys_free    free

void* scf_malloc(int size)
{
	scf_object_t* obj = __sys_malloc(sizeof(scf_object_t) + size);
	if (!obj)
		return NULL;

	scf_atomic_inc(&obj->refs);

	scf_loge("obj: %p\n", obj);
	return obj->data;
}

void scf_free(void* data)
{
	if (!data)
		return;

	scf_object_t* obj = (scf_object_t*)((uint8_t*)data - sizeof(scf_object_t));

	if (scf_atomic_dec_and_test(&obj->refs)) {
		scf_loge("obj: %p\n", obj);
		__sys_free(obj);
	}
}

void scf_ref(void* data)
{
	if (!data)
		return;

	scf_object_t* obj = (scf_object_t*)((uint8_t*)data - sizeof(scf_object_t));

	scf_atomic_inc(&obj->refs);
}

#if 0
int main()
{
	int* a = scf_malloc(sizeof(int) * 10);
	if (!a) {
		scf_loge("\n");
		return -1;
	}

	scf_loge("a: %p\n", a);

	int i;
	for (i = 9; i >= 0; i--)
		a[i] = i;

	for (i = 0; i < 10; i++)
		printf("%d\n", a[i]);

	scf_free(a);
	return 0;
}

#endif
