#ifndef SCF_STACK_H
#define SCF_STACK_H

#include"scf_vector.h"

typedef  scf_vector_t  scf_stack_t;

static inline scf_stack_t* scf_stack_alloc()
{
	return scf_vector_alloc();
}

static inline int scf_stack_push(scf_stack_t* s, void* node)
{
	return scf_vector_add(s, node);
}

static inline void* scf_stack_pop(scf_stack_t* s)
{
	if (!s || !s->data)
		return NULL;

	assert(s->size >= 0);

	if (0 == s->size)
		return NULL;

	void* node = s->data[--s->size];

	if (s->size + NB_MEMBER_INC * 2 < s->capacity) {
		void* p = realloc(s->data, sizeof(void*) * (s->capacity - NB_MEMBER_INC));
		if (p) {
			s->data = p;
			s->capacity -= NB_MEMBER_INC;
		}
	}

	return node;
}

static inline void* scf_stack_top(scf_stack_t* s)
{
	if (!s || !s->data)
		return NULL;

	assert(s->size >= 0);

	if (0 == s->size)
		return NULL;

	return s->data[s->size - 1];
}

static inline void scf_stack_free(scf_stack_t* s)
{
	scf_vector_free(s);
}

#endif

