
struct scf_object_t
{
	intptr_t   refs;
	uintptr_t  size;
};

int   scf_printf(const char* fmt, ...);

void* scf__malloc (uintptr_t size);
void* scf__calloc (uintptr_t n, uintptr_t size);
void* scf__realloc(void* p,     uintptr_t size);
int   scf__free   (void* p);

void  scf__atomic_inc(intptr_t* p);
void  scf__atomic_dec(intptr_t* p);
int   scf__atomic_dec_and_test(intptr_t* p);

void  scf__release_pt(void* objdata);

void* scf__auto_malloc(uintptr_t size);

void scf__auto_freep(void** pp, scf__release_pt* release);

void scf__auto_freep_array(void** pp, int nb_pointers, scf__release_pt* release);

void scf__auto_free_array(void** pp, int size, int nb_pointers, scf__release_pt* release);

void scf__auto_ref(void* data);


int main()
{
	int** pp = scf__auto_malloc(sizeof(int*) * 4);

	scf_printf("pp: %p\n\n", pp);

	int i;
	for (i = 0; i < 4; i++) {
		int* p  = scf__auto_malloc(sizeof(int));

		pp[i] = p;
		scf_printf("\n");
	}

	return 0;
}

