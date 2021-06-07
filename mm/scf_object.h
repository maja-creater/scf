#ifndef SCF_OBJECT_H
#define SCF_OBJECT_H

#include"scf_atomic.h"

typedef struct {
	scf_atomic_t  refs;
	int           size;
	uint8_t       data[0];
} scf_object_t;

void* scf_malloc(int size);

void  scf_freep(void** pp, void (*release)(void* objdata));

void  scf_free_array (void** pp, int size, int nb_pointers, void (*release)(void* objdata));
void  scf_freep_array(void** pp, int nb_pointers, void (*release)(void* objdata));

void  scf_ref (void* data);

#endif

