#ifndef SCF_STRING_H
#define SCF_STRING_H

#include"scf_def.h"

typedef struct {
	int      capacity;
	size_t   len;
	char*    data;
} scf_string_t;


scf_string_t*	scf_string_alloc();

scf_string_t*	scf_string_clone(scf_string_t* s);

scf_string_t*	scf_string_cstr(const char* str);

scf_string_t*	scf_string_cstr_len(const char* str, size_t len);

void 			scf_string_free(scf_string_t* s);

int				scf_string_cmp(const scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cmp_cstr(const scf_string_t* s0, const char* str);

int				scf_string_cmp_cstr_len(const scf_string_t* s0, const char* str, size_t len);

int				scf_string_copy(scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cat(scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cat_cstr(scf_string_t* s0, const char* str);

int				scf_string_cat_cstr_len(scf_string_t* s0, const char* str, size_t len);

#endif

