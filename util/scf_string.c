#include"scf_string.h"

#define SCF_STRING_NUMBER_INC	4

scf_string_t*	scf_string_alloc()
{
	scf_string_t* s = malloc(sizeof(scf_string_t));
	if (!s) {
		return NULL;
	}

	s->data = malloc(SCF_STRING_NUMBER_INC + 1);
	if (!s->data) {
		free(s);
		return NULL;
	}

	s->capacity = SCF_STRING_NUMBER_INC;
	s->len = 0;
	s->data[0] = '\0';
	return s;
}

scf_string_t*	scf_string_clone(scf_string_t* s)
{
	if (!s) {
		return NULL;
	}

	scf_string_t* s1 = malloc(sizeof(scf_string_t));
	if (!s1) {
		return NULL;
	}

	if (s->capacity > 0) {
		s1->capacity = s->capacity;
	} else if (s->len > 0) {
		s1->capacity = s->len;
	} else {
		s1->capacity = SCF_STRING_NUMBER_INC;
	}

	s1->data = malloc(s1->capacity + 1);
	if (!s1->data) {
		free(s1);
		return NULL;
	}

	s1->len = s->len;
	if (s->len > 0) {
		memcpy(s1->data, s->data, s->len);
	}
	s1->data[s1->len] = '\0';

	return s1;
}

scf_string_t*	scf_string_cstr(const char* str)
{
	if (!str) {
		return NULL;
	}

	return scf_string_cstr_len(str, strlen(str));
}

scf_string_t*	scf_string_cstr_len(const char* str, size_t len)
{
	if (!str) {
		return NULL;
	}

	scf_string_t s;
	s.capacity	= -1;
	s.len		= len;
	s.data		= (char*)str;

	return scf_string_clone(&s);
}

void scf_string_free(scf_string_t* s)
{
	if (!s) {
		return;
	}

	if (s->capacity > 0) {
		free(s->data);
	}

	free(s);
	s = NULL;
}

int	scf_string_cmp(const scf_string_t* s0, const scf_string_t* s1)
{
	assert(s0);
	assert(s1);

	if (s0->len < s1->len) {
		return -1;
	} else if (s0->len > s1->len) {
		return 1;
	} else {
		return strncmp(s0->data, s1->data, s0->len);
	}
}

int	scf_string_cmp_cstr(const scf_string_t* s0, const char* str)
{
	assert(s0);
	assert(str);

	return scf_string_cmp_cstr_len(s0, str, strlen(str));
}

int	scf_string_cmp_cstr_len(const scf_string_t* s0, const char* str, size_t len)
{
	assert(s0);
	assert(str);

	scf_string_t s1;
	s1.capacity	= -1;
	s1.len		= len;
	s1.data		= (char*)str;

	return scf_string_cmp(s0, &s1);
}

int	scf_string_copy(scf_string_t* s0, const scf_string_t* s1)
{
	assert(s0);
	assert(s1);
	assert(s0->capacity > 0);

	if (s1->len > s0->capacity) {
		char* p = realloc(s0->data, s1->len + 1);
		if (!p) {
			return -1;
		}

		s0->data = p;
		s0->capacity = s1->len;
	}

	memcpy(s0->data, s1->data, s1->len);
	s0->data[s1->len] = '\0';
	s0->len = s1->len;
	return 0;
}

int	scf_string_cat(scf_string_t* s0, const scf_string_t* s1)
{
	assert(s0);
	assert(s1);
	assert(s0->capacity > 0);

	if (s0->len + s1->len > s0->capacity) {
		char* p = realloc(s0->data, s0->len + s1->len + SCF_STRING_NUMBER_INC + 1);
		if (!p) {
			return -1;
		}

		s0->data = p;
		s0->capacity = s0->len + s1->len + SCF_STRING_NUMBER_INC;
	}

	memcpy(s0->data + s0->len, s1->data, s1->len);
	s0->data[s0->len + s1->len] = '\0';
	s0->len += s1->len;
	return 0;
}

int	scf_string_cat_cstr(scf_string_t* s0, const char* str)
{
	assert(s0);
	assert(str);

	return scf_string_cat_cstr_len(s0, str, strlen(str));
}

int	scf_string_cat_cstr_len(scf_string_t* s0, const char* str, size_t len)
{
	assert(s0);
	assert(str);

	scf_string_t s1;
	s1.capacity	= -1;
	s1.len		= len;
	s1.data		= (char*)str;

	return scf_string_cat(s0, &s1);
}

#if 0
int main(int argc, char* argv[])
{
	scf_string_t* s0 = scf_string_alloc();
	scf_string_t* s1 = scf_string_cstr("hello, world!");
	scf_string_t* s2 = scf_string_clone(s1);
	printf("s0: %s\n", s0->data);
	printf("s1: %s\n", s1->data);
	printf("s2: %s\n", s2->data);

	scf_string_copy(s0, s2);
	printf("s0: %s\n", s0->data);

	printf("s0 cmp s1: %d\n", scf_string_cmp(s0, s1));

	scf_string_t* s3 = scf_string_cstr("ha ha ha!");
	printf("s3: %s\n", s3->data);

	scf_string_cat(s0, s3);
	printf("s0: %s\n", s0->data);

	scf_string_cat_cstr(s0, "he he he!");
	printf("s0: %s\n", s0->data);

	scf_string_free(s0);
	scf_string_free(s1);
	scf_string_free(s2);
	scf_string_free(s3);
	return 0;
}
#endif

