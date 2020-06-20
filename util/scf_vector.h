#ifndef SCF_VECTOR_H
#define SCF_VECTOR_H

#include"scf_def.h"

typedef struct {
	int		capacity;
	int		size;
	void**	data;
} scf_vector_t;

#undef  NB_MEMBER_INC
#define NB_MEMBER_INC	16

static inline scf_vector_t* scf_vector_alloc()
{
	scf_vector_t* v = calloc(1, sizeof(scf_vector_t));
	assert(v);

	v->data = calloc(NB_MEMBER_INC, sizeof(void*));
	assert(v->data);

	v->capacity = NB_MEMBER_INC;
	return v;
}

static inline scf_vector_t* scf_vector_clone(scf_vector_t* origin)
{
	scf_vector_t* clone = calloc(1, sizeof(scf_vector_t));
	assert(clone);

	clone->data = calloc(origin->capacity, sizeof(void*));
	assert(clone->data);

	clone->capacity = origin->capacity;
	clone->size		= origin->size;
	memcpy(clone->data, origin->data, origin->size * sizeof(void*));
	return clone;
}

static inline void scf_vector_add(scf_vector_t* v, void* node)
{
	assert(v);
	assert(v->size <= v->capacity);

	if (v->size == v->capacity) {
		void* p = realloc(v->data, sizeof(void*) * (v->capacity + NB_MEMBER_INC));
		assert(p);
		v->data = p;
		v->capacity += NB_MEMBER_INC;
	}

	v->data[v->size++] = node;
}

static inline int scf_vector_del(scf_vector_t* v, void* node)
{
	assert(v);
	assert(v->size <= v->capacity);

	int i;
	for (i = 0; i < v->size; i++) {
		if (v->data[i] == node) {
			int j;
			for (j = i + 1; j < v->size; j++) {
				v->data[j - 1] = v->data[j];
			}
			v->size--;

			if (v->size + NB_MEMBER_INC * 2 < v->capacity) {
				void* p = realloc(v->data, sizeof(void*) * (v->capacity - NB_MEMBER_INC));
				assert(p);
				v->data = p;
				v->capacity -= NB_MEMBER_INC;
			}
			return 0;
		}
	}

	return -1;
}

static inline void* scf_vector_find(const scf_vector_t* v, const void* node)
{
	int i;
	for (i = 0; i < v->size; i++) {
		if (node == v->data[i])
			return v->data[i];
	}
	return NULL;
}

static inline void* scf_vector_find_cmp(const scf_vector_t* v, const void* node, int (*cmp)(const void*, const void*))
{
	int i;
	for (i = 0; i < v->size; i++) {
		if (0 == cmp(node, v->data[i]))
			return v->data[i];
	}
	return NULL;
}

static inline void scf_vector_clear(scf_vector_t* v, void (*type_free)(void*))
{
	if (v && v->data) {
		int i;
		for (i = 0; i < v->size; i++) {
			if (type_free)
				type_free(v->data[i]);

			v->data[i] = NULL;
		}
	}
}

static inline void scf_vector_free(scf_vector_t* v)
{
	assert(v);
	assert(v->data);

	free(v->data);
	free(v);
}

#endif

