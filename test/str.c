
int scf_printf(const char* fmt, ...);

uintptr_t scf__strlen (const char *s);
int       scf__strcmp (const char *s1, const char *s2);
int       scf__strncmp(const char *s1, const char *s2,  uintptr_t n);
char*     scf__strncpy(char *dst,      const char *src, uintptr_t n);

int main()
{
	char  buf[100];
	char* s0 = "hello1";
	char* s1 = "hello2";

	scf_printf("%s, len: %d\n", s0, scf__strlen(s0));

	scf_printf("%s, %s, cmp: %d\n", s0, s1, scf__strcmp(s0, s1));
	scf_printf("%s, %s, cmp: %d\n", s1, s0, scf__strcmp(s1, s0));
	scf_printf("%s, %s, cmp: %d\n", s0, s0, scf__strcmp(s0, s0));

	scf_printf("%s, %s, cmp: %d, n: %d\n", s0, s1, scf__strncmp(s0, s1, 5), 5);

	scf__strncpy(buf, s0, 5);
	scf_printf("buf: %s\n", buf);

	scf__strncpy(buf, s0, 6);
	scf_printf("buf: %s\n", buf);

	return 0;
}

