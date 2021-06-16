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
	if (!v)
		return NULL;

	v->data = calloc(NB_MEMBER_INC, sizeof(void*));
	if (!v->data) {
		free(v);
		v = NULL;
		return NULL;
	}

	v->capacity = NB_MEMBER_INC;
	return v;
}

static inline scf_vector_t* scf_vector_clone(scf_vector_t* origin)
{
	scf_vector_t* clone = calloc(1, sizeof(scf_vector_t));
	if (!clone)
		return NULL;

	clone->data = calloc(origin->capacity, sizeof(void*));
	if (!clone->data) {
		free(clone);
		clone = NULL;
		return NULL;
	}

	clone->capacity = origin->capacity;
	clone->size		= origin->size;
	memcpy(clone->data, origin->data, origin->size * sizeof(void*));
	return clone;
}

static inline int scf_vector_cat(scf_vector_t* dst, scf_vector_t* src)
{
	if (!dst || !src)
		return -EINVAL;

	int size = dst->size + src->size;
	if (size > dst->capacity) {

		void* p = realloc(dst->data, sizeof(void*) * (size + NB_MEMBER_INC));
		if (!p)
			return -ENOMEM;

		dst->data = p;
		dst->capacity = size + NB_MEMBER_INC;
	}

	memcpy(dst->data + dst->size * sizeof(void*), src->data, src->size * sizeof(void*));
	dst->size += src->size;
	return 0;
}

static inline int scf_vector_add(scf_vector_t* v, void* node)
{
	if (!v || !v->data)
		return -EINVAL;

	assert(v->size <= v->capacity);

	if (v->size == v->capacity) {
		void* p = realloc(v->data, sizeof(void*) * (v->capacity + NB_MEMBER_INC));
		if (!p)
			return -ENOMEM;

		v->data = p;
		v->capacity += NB_MEMBER_INC;
	}

	v->data[v->size++] = node;
	return 0;
}

static inline int scf_vector_del(scf_vector_t* v, void* node)
{
	if (!v || !v->data)
		return -EINVAL;

	assert(v->size <= v->capacity);

	int i;
	for (i = 0; i < v->size; i++) {

		if (v->data[i] != node)
			continue;

		int j;
		for (j = i + 1; j < v->size; j++)
			v->data[j - 1] = v->data[j];

		v->size--;

		if (v->size + NB_MEMBER_INC * 2 < v->capacity) {
			void* p = realloc(v->data, sizeof(void*) * (v->capacity - NB_MEMBER_INC));
			if (p) {
				v->data = p;
				v->capacity -= NB_MEMBER_INC;
			}
		}
		return 0;
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

static inline int scf_vector_add_unique(scf_vector_t* v, void* node)
{
	if (!scf_vector_find(v, node))
		return scf_vector_add(v, node);
	return 0;
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

static inline int scf_vector_qsort(const scf_vector_t* v, int (*cmp)(const void*, const void*))
{
	if (!v || !v->data || 0 == v->size || !cmp)
		return -EINVAL;

	qsort(v->data, v->size, sizeof(void*), cmp);
	return 0;
}

static inline void scf_vector_clear(scf_vector_t* v, void (*type_free)(void*))
{
	if (!v || !v->data)
		return;

	if (type_free) {
		int i;
		for (i = 0; i < v->size; i++) {
			if (v->data[i]) {
				type_free(v->data[i]);
				v->data[i] = NULL;
			}
		}
	}

	v->size = 0;

	if (v->capacity > NB_MEMBER_INC) {
		void* p = realloc(v->data, sizeof(void*) * NB_MEMBER_INC);
		if (p) {
			v->data = p;
			v->capacity = NB_MEMBER_INC;
		}
	}
}

static inline void scf_vector_free(scf_vector_t* v)
{
	if (v) {
		if (v->data)
			free(v->data);

		free(v);
		v = NULL;
	}
}

#endif

