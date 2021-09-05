#include"scf_parse.h"
#include"scf_3ac.h"
#include"scf_x64.h"
#include"scf_elf_link.h"

static char* __objs[] = {
	"../lib/_start.o",
	"../lib/scf_syscall.o",
	"../lib/scf_linux_api.o",
	"../lib/scf_object.o",
	"../lib/scf_atomic.o",
};

static char* __sofiles[] = {
	"/lib64/ld-linux-x86-64.so.2",
	"/lib/x86_64-linux-gnu/libc.so.6",
};

int main(int argc, char* argv[])
{
	int   opt;
	int   link = 1;
	char* out  = NULL;

	while ((opt = getopt(argc, argv, "co:")) != -1) {
		switch (opt) {
			case 'c':
				link = 0;
				break;
			case 'o':
				out = optarg;
				break;
			default:
				fprintf(stderr, "Usage: %s src0 [src1] [-o out]\n", argv[0]);
				break;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		return -1;
	}

	scf_vector_t* afiles  = scf_vector_alloc();
	scf_vector_t* sofiles = scf_vector_alloc();
	scf_vector_t* srcs    = scf_vector_alloc();
	scf_vector_t* objs    = scf_vector_alloc();

	while (optind < argc) {
		char*  fname = argv[optind];
		size_t len   = strlen(fname);

		if (len < 3) {
			fprintf(stderr, "file '%s' invalid\n", fname);
			return -1;
		}

		scf_vector_t* vec;

		if (!strcmp(fname + len - 2, ".a"))
			vec = afiles;

		else if (!strcmp(fname + len - 2, ".o"))
			vec = objs;

		else if (!strcmp(fname + len - 2, ".c"))
			vec = srcs;

		else if (!strcmp(fname + len - 3, ".so"))
			vec = sofiles;
		else {
			fprintf(stderr, "file '%s' invalid\n", fname);
			return -1;
		}

		if (scf_vector_add(vec, fname) < 0)
			return -ENOMEM;

		optind++;
	}

	scf_parse_t*  parse = NULL;

	if (scf_parse_open(&parse) < 0) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i  < srcs->size; i++) {
		char* file = srcs->data[i];

		if (scf_parse_file(parse, file) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	char* obj  = "1.elf";
	char* exec = "1.out";

	if (out) {
		if (!link)
			obj  = out;
		else
			exec = out;
	}

	if (scf_parse_compile(parse, obj) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_parse_close(parse);

	if (!link) {
		printf("%s(),%d, main ok\n", __func__, __LINE__);
		return 0;
	}


	for (i  = 0; i < sizeof(__objs) / sizeof(__objs[0]); i++) {

		int ret = scf_vector_add(objs, __objs[i]);
		if (ret < 0)
			return ret;
	}

	for (i  = 0; i < sizeof(__sofiles) / sizeof(__sofiles[0]); i++) {

		int ret = scf_vector_add(sofiles, __sofiles[i]);
		if (ret < 0)
			return ret;
	}

	if (scf_vector_add(objs, obj) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (scf_elf_link(objs, afiles, sofiles, exec) < 0) {
		scf_loge("\n");
		return -1;
	}

	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}

