
int scf_printf(const char* fmt, ...);

uintptr_t scf__strlen(const char *s)
{
	uintptr_t i = 0;

	while (*s) {
		s++;
		i++;
	}

	return i;
}

int scf__strcmp(const char *s1, const char *s2)
{
	if (s1 == s2)
		return 0;

	while (*s1 && *s2) {

		if (*s1 < *s2)
			return -1;
		else if (*s1 > *s2)
			return 1;

		s1++;
		s2++;
	}

	if (*s1 < *s2)
		return -1;
	else if (*s1 > *s2)
		return 1;
	return 0;
}

int scf__strncmp(const char *s1, const char *s2, uintptr_t n)
{
	if (s1 == s2)
		return 0;

	uintptr_t i = 0;

	while (*s1 && *s2 && i < n) {

		if (*s1 < *s2)
			return -1;
		else if (*s1 > *s2)
			return 1;

		i++;
		s1++;
		s2++;
	}

	if (i == n)
		return 0;
	else if (*s1 > *s2)
		return 1;
	return -1;
}

char* scf__strncpy(char *dst, const char *src, uintptr_t n)
{
	uintptr_t i = 0;

	for (i = 0; i < n && src[i] != '\0'; i++)
		dst[i] = src[i];

	for ( ; i < n; i++)
		dst[i] = '\0';

	return dst;
}

