#include"scf_parse.h"
#include"scf_3ac.h"
#include"scf_x64.h"

int main(int argc, char* argv[])
{
	scf_parse_t* parse = NULL;

	if (scf_parse_open(&parse, argv[1]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	int64_t tv0 = gettime();

	if (scf_parse_parse(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	int64_t tv1 = gettime();

	printf("\n");
#if 1
	if (scf_parse_compile(parse, argv[1]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	int64_t tv2 = gettime();
	printf("\n");
#endif

	scf_logw("tv1 - tv0: %ld\n", tv1 - tv0);
	scf_logw("tv2 - tv1: %ld\n", tv2 - tv1);

#if 0
	if (!scf_list_empty(&parse->code_list_head)) {
		scf_3ac_code_print(&parse->code_list_head);
	}
	printf("\n");
#endif

	scf_parse_close(parse);
	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}
