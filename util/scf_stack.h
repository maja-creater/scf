#ifndef SCF_STACK_H
#define SCF_STACK_H

#include"scf_def.h"

typedef struct {
	int		capacity;
	int		size;
	void**	data;
} scf_stack_t;

#undef  NB_MEMBER_INC
#define NB_MEMBER_INC	16

static inline scf_stack_t* scf_stack_alloc()
{
	scf_stack_t* s = calloc(1, sizeof(scf_stack_t));
	assert(s);

	s->data = calloc(NB_MEMBER_INC, sizeof(void*));
	assert(s->data);

	s->capacity = NB_MEMBER_INC;
	return s;
}

static inline void scf_stack_push(scf_stack_t* s, void* node)
{
	assert(s);
	assert(s->size <= s->capacity);

	if (s->size == s->capacity) {
		void* p = realloc(s->data, sizeof(void*) * (s->capacity + NB_MEMBER_INC));
		assert(p);
		s->data = p;
		s->capacity += NB_MEMBER_INC;
	}

	s->data[s->size++] = node;
}

static inline void* scf_stack_pop(scf_stack_t* s)
{
	assert(s);
	assert(s->size >= 0);

	if (0 == s->size)
		return NULL;

	void* node = s->data[--s->size];

	if (s->size + NB_MEMBER_INC * 2 < s->capacity) {
		void* p = realloc(s->data, sizeof(void*) * (s->capacity - NB_MEMBER_INC));
		assert(p);
		s->data = p;
		s->capacity -= NB_MEMBER_INC;
	}

	return node;
}

static inline void* scf_stack_top(scf_stack_t* s)
{
	assert(s);
	assert(s->size >= 0);

	if (0 == s->size)
		return NULL;

	return s->data[s->size - 1];
}

static inline void scf_stack_free(scf_stack_t* s)
{
	assert(s);
	assert(s->data);

	free(s->data);
	free(s);
}

#endif

