
struct scf_object_t
{
	intptr_t   refs;
	uintptr_t  size;
};

int   printf(const char* fmt, ...);

void* malloc (uintptr_t size);
void* calloc (uintptr_t n, uintptr_t size);
void* realloc(void* p,     uintptr_t size);
int   free   (void* p);

void  scf__atomic_inc(intptr_t* p);
void  scf__atomic_dec(intptr_t* p);
int   scf__atomic_dec_and_test(intptr_t* p);

void  scf__release_pt(void* objdata);


void* scf__auto_malloc(uintptr_t size)
{
	scf_object_t* obj = calloc(1, sizeof(scf_object_t) + size);
	if (!obj)
		return NULL;

	scf__atomic_inc(&obj->refs);
	obj->size = size;

	void* data = (void*)obj + sizeof(scf_object_t);

	printf("%s(), obj: %p, size: %d\n", __func__, obj, size);
	return data;
}

void scf__auto_freep(void** pp, scf__release_pt* release)
{
	if (!pp || !*pp) {
		printf("%s()\n\n", __func__);
		return;
	}

	void*         data = *pp;
	scf_object_t* obj  = data - sizeof(scf_object_t);

	if (scf__atomic_dec_and_test(&obj->refs)) {

		printf("%s(), obj: %p\n", __func__, obj);

		if (release)
			release(data);

		free(obj);
	} else {
		printf("%s(), obj: %p, refs: %ld\n", __func__, obj, obj->refs);
	}

	*pp = NULL;
}

void scf__auto_freep_array(void** pp, int nb_pointers, scf__release_pt* release)
{
	if (!pp || !*pp)
		return;

	scf_object_t* obj;

	void** p = (void**) *pp;

	if (nb_pointers > 1) {
		void*    data;
		intptr_t size;
		intptr_t i;

		obj  = (void*)p  - sizeof(scf_object_t);

		size = obj->size / sizeof(void*);

		printf("%d, size: %ld\n", __LINE__, size);

		for (i   = 0; i < size; i++) {
			data = p[i];

			if (!data)
				continue;

			obj  = data - sizeof(scf_object_t);

			scf__auto_freep_array(&p[i], nb_pointers - 1, release);
			p[i] = NULL;
		}
	}

	obj  = (void*)p - sizeof(scf_object_t);

//	printf("%s(), pp: %p, p: %p, nb_pointers: %d\n\n", __func__, pp, p, nb_pointers);

	if (scf__atomic_dec_and_test(&obj->refs)) {

		if (release && 1 == nb_pointers)
			release(p);

		printf("%s(), obj: %p\n\n", __func__, obj);

		free(obj);
	}

	*pp = NULL;
}

void scf__auto_free_array(void** pp, int size, int nb_pointers, scf__release_pt* release)
{
	if (!pp)
		return;

	scf_object_t* obj;
	void*         data;

	int i;
	for (i = 0; i < size; i++) {

		data = pp[i];

		if (!data)
			continue;

		obj  = data - sizeof(scf_object_t);

		if (scf__atomic_dec_and_test(&obj->refs)) {

			printf("%s(), obj: %p, pp: %p, i: %d, nb_pointers: %d\n", __func__, obj, pp, i, nb_pointers);

			if (nb_pointers > 1)

				scf__auto_freep_array(&pp[i], nb_pointers - 1, release);
			else {
				if (release)
					release(data);

				free(obj);
			}
		}

		pp[i] = NULL;
	}
}

void scf__auto_ref(void* data)
{
	if (data) {

		scf_object_t* obj = data - sizeof(scf_object_t);

		printf("%s(), obj: %p\n", __func__, obj);

		scf__atomic_inc(&obj->refs);
	}
}

