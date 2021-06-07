#ifndef SCF_ATOMIC_H
#define SCF_ATOMIC_H

#include"scf_def.h"

typedef struct {
	volatile int  count;
} scf_atomic_t;

static inline void scf_atomic_inc(scf_atomic_t* v)
{
	asm volatile(
		"lock; incl %0"
		:"=m"(v->count)
		:
		:
	);
}

static inline void scf_atomic_dec(scf_atomic_t* v)
{
	asm volatile(
		"lock; decl %0"
		:"=m"(v->count)
		:
		:
	);
}

static inline int scf_atomic_dec_and_test(scf_atomic_t* v)
{
	unsigned char ret;

	asm volatile(
		"lock; decl %0\r\n"
		"setz %1\r\n"
		:"=m"(v->count), "=r"(ret)
		:
		:
	);

	return ret;
}

#endif

