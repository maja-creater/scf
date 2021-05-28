#ifndef SCF_OBJECT_H
#define SCF_OBJECT_H

#include"scf_atomic.h"

typedef struct {
	scf_atomic_t  refs;
	uint8_t       data[0];
} scf_object_t;

void* scf_malloc(int size);

void  scf_free(void* data, void (*release)(void* objdata));

void  scf_ref (void* data);

#endif

