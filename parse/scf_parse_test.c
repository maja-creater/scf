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

	if (scf_parse_parse(parse) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	printf("\n");
#if 1
	if (scf_parse_compile(parse, argv[1]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}
	printf("\n");
#endif

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
