#include"../scf_atomic.h"

scf_atomic_t v = {0};

int main()
{
	scf_atomic_inc(&v);
	scf_loge("v.count: %d\n", v.count);

	scf_atomic_inc(&v);
	scf_loge("v.count: %d\n", v.count);

	scf_atomic_dec(&v);
	scf_loge("v.count: %d\n", v.count);

	int ret = scf_atomic_dec_and_test(&v);
	scf_loge("v.count: %d, ret: %d\n", v.count, ret);

	scf_loge("sizeof(v): %ld\n", sizeof(v));
	return 0;
}
