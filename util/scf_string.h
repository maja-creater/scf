#ifndef SCF_STRING_H
#define SCF_STRING_H

#include"scf_vector.h"

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

void 			scf_string_print_bin(scf_string_t* s);

int	            scf_string_fill_zero(scf_string_t* s0, size_t len);

int				scf_string_cmp(const scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cmp_cstr(const scf_string_t* s0, const char* str);

int				scf_string_cmp_cstr_len(const scf_string_t* s0, const char* str, size_t len);

int				scf_string_copy(scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cat(scf_string_t* s0, const scf_string_t* s1);

int				scf_string_cat_cstr(scf_string_t* s0, const char* str);

int             scf_string_cat_cstr_len(scf_string_t* s0, const char* str, size_t len);

int             scf_string_match_kmp(const scf_string_t* T, const scf_string_t* P, scf_vector_t* offsets);
int             scf_string_match_kmp_cstr(const uint8_t* T, const uint8_t* P,      scf_vector_t* offsets);
int             scf_string_match_kmp_cstr_len(const scf_string_t* T, const uint8_t* P, size_t Plen, scf_vector_t* offsets);

int             scf_string_get_offset(scf_string_t* str, const char* data, size_t len);

#endif

