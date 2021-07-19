
#include<stdio.h>
#include<stdint.h>
#include<pthread.h>

int64_t v = 0;

void scf__atomic_inc(int64_t* p);
void scf__atomic_dec(int64_t* p);
int  scf__atomic_dec_and_test(int64_t* p);

void* thread(void* p)
{
	int64_t *pi = p;
	int i;
	for (i = 0; i < 1000 * 1000; i++)
		scf__atomic_inc(p);
	return NULL;
}

int main()
{
	scf__atomic_inc(&v);
	printf("v: %ld\n", v);

	scf__atomic_inc(&v);
	printf("v: %ld\n", v);

	scf__atomic_dec(&v);
	printf("v: %ld\n", v);

	int ret = scf__atomic_dec_and_test(&v);
	printf("v: %ld, ret: %d\n", v, ret);

	pthread_t tid;
	pthread_create(&tid, NULL, thread, &v);

	int i;
	for (i = 0; i < 1000 * 1000; i++)
		scf__atomic_inc(&v);

	pthread_join(tid, NULL);

	printf("v: %ld\n", v);
	return 0;
}
