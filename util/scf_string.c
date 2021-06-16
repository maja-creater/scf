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

void scf_string_print_bin(scf_string_t* s)
{
	if (!s)
		return;

	int i;
	for (i = 0; i < s->len; i++) {

		unsigned char c = s->data[i];

		if (i > 0 && i % 10 == 0)
			printf("\n");

		printf("%#02x ", c);
	}
	printf("\n");
}

int	scf_string_cmp(const scf_string_t* s0, const scf_string_t* s1)
{
	if (!s0 || !s1 || !s0->data || !s1->data)
		return -EINVAL;

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
	if (!s0 || !s0->data || !str)
		return -EINVAL;

	return scf_string_cmp_cstr_len(s0, str, strlen(str));
}

int	scf_string_cmp_cstr_len(const scf_string_t* s0, const char* str, size_t len)
{
	if (!s0 || !s0->data || !str)
		return -EINVAL;

	scf_string_t s1;
	s1.capacity	= -1;
	s1.len		= len;
	s1.data		= (char*)str;

	return scf_string_cmp(s0, &s1);
}

int	scf_string_copy(scf_string_t* s0, const scf_string_t* s1)
{
	if (!s0 || !s1 || !s0->data || !s1->data)
		return -EINVAL;

	assert(s0->capacity > 0);

	if (s1->len > s0->capacity) {
		char* p = realloc(s0->data, s1->len + 1);
		if (!p)
			return -ENOMEM;

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
	if (!s0 || !s1 || !s0->data || !s1->data) {
		scf_loge("\n");
		return -EINVAL;
	}

	assert(s0->capacity > 0);

	if (s0->len + s1->len > s0->capacity) {
		char* p = realloc(s0->data, s0->len + s1->len + SCF_STRING_NUMBER_INC + 1);
		if (!p) {
		scf_loge("\n");
			return -ENOMEM;
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
	if (!s0 || !s0->data || !str)
		return -EINVAL;

	return scf_string_cat_cstr_len(s0, str, strlen(str));
}

int	scf_string_cat_cstr_len(scf_string_t* s0, const char* str, size_t len)
{
	if (!s0 || !s0->data || !str) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_string_t s1;
	s1.capacity	= -1;
	s1.len		= len;
	s1.data		= (char*)str;

	return scf_string_cat(s0, &s1);
}

int	scf_string_fill_zero(scf_string_t* s0, size_t len)
{
	if (!s0 || !s0->data)
		return -EINVAL;

	assert(s0->capacity > 0);

	if (s0->len + len > s0->capacity) {

		char* p = realloc(s0->data, s0->len + len + SCF_STRING_NUMBER_INC + 1);
		if (!p)
			return -ENOMEM;

		s0->data = p;
		s0->capacity = s0->len + len + SCF_STRING_NUMBER_INC;
	}

	memset(s0->data + s0->len, 0, len);
	s0->data[s0->len + len] = '\0';
	s0->len += len;
	return 0;
}

static int* _prefix_kmp(const uint8_t* P, int m)
{
	int* prefix = malloc(sizeof(int) * (m + 1));
	if (!prefix)
		return NULL;

	prefix[0] = -1;

	int k = -1;
	int q;

	for (q = 1; q < m; q++) {

		while (k > -1 && P[k + 1] != P[q])
			k = prefix[k];

		if (P[k + 1] == P[q])
			k++;

		prefix[q] = k;
	}
	return prefix;
}

static int _match_kmp(const uint8_t* T, int Tlen, const uint8_t* P, int Plen, scf_vector_t* offsets)
{
	if (Tlen <= 0 || Plen <= 0)
		return -EINVAL;

	int n = Tlen;
	int m = Plen;

	int* prefix = _prefix_kmp(P, m);
	if (!prefix)
		return -1;

	int q = -1;
	int i;

	for (i = 0; i < n; i++) {

		while (q > -1 && P[q + 1] != T[i])
			q = prefix[q];

		if (P[q + 1] == T[i])
			q++;

		if (q == m - 1) {
			scf_logd("KMP find P: %s in T: %s, offset: %d\n", P, T, i - m + 1);

			int ret = scf_vector_add(offsets, (void*)(intptr_t)(i - m + 1));
			if (ret < 0)
				return ret;

			q = prefix[q];
		}
	}

	free(prefix);
	return 0;
}

int scf_string_match_kmp(const scf_string_t* T, const scf_string_t* P, scf_vector_t* offsets)
{
	if (!T || !P || !offsets)
		return -EINVAL;

	return _match_kmp(T->data, T->len, P->data, P->len, offsets);
}

int scf_string_match_kmp_cstr(const uint8_t* T, const uint8_t* P, scf_vector_t* offsets)
{
	if (!T || !P || !offsets)
		return -EINVAL;

	return _match_kmp(T, strlen(T), P, strlen(P), offsets);
}

int scf_string_match_kmp_cstr_len(const scf_string_t* T, const uint8_t* P, size_t Plen, scf_vector_t* offsets)
{
	if (!T || !P || 0 == Plen || !offsets)
		return -EINVAL;

	return _match_kmp(T->data, T->len, P, Plen, offsets);
}

int scf_string_get_offset(scf_string_t* str, const char* data, size_t len)
{
	int ret;

	if (0 == str->len) {

		if (scf_string_cat_cstr_len(str, data, len) < 0)
			return -1;
		return 0;
	}

	scf_vector_t* vec = scf_vector_alloc();
	if (!vec)
		return -ENOMEM;

	ret = scf_string_match_kmp_cstr_len(str, data, len, vec);
	if (ret < 0) {

	} else if (0 == vec->size) {
		ret = str->len;

		if (scf_string_cat_cstr_len(str, data, len) < 0)
			ret = -1;
	} else
		ret = (intptr_t)(vec->data[0]);

	scf_vector_free(vec);
	return ret;
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

	scf_string_t* s4  = scf_string_cstr("hello, world!");
	scf_string_t* s5  = scf_string_cstr("ll");
	scf_vector_t* vec = scf_vector_alloc();

	int ret = scf_string_match_kmp(s4, s5, vec);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i < vec->size; i++) {

		int offset = (intptr_t)vec->data[i];

		printf("i: %d, offset: %d\n", i, offset);
	}

	scf_string_free(s0);
	scf_string_free(s1);
	scf_string_free(s2);
	scf_string_free(s3);
	return 0;
}
#endif

