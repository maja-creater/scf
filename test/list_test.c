struct list
{
	list* prev;
	list* next;
};

inline void  list_init(list* h);
inline list* list_head(list* h);
inline list* list_tail(list* h);
inline list* list_sentinel(list* h);
inline list* list_next(list* n);
inline list* list_prev(list* n);
inline void  list_del(list* n);
inline int   list_empty(list* h);
inline void  list_add_tail(list* h, list* n);
inline void  list_add_front(list* h, list* n);

struct Data
{
	list list;
	int  a;
};

void* scf__malloc(uintptr_t size);
int   scf__free(void* p);
int   scf_printf(const char* fmt, ...);

int main()
{
	Data* d;
	list  h;
	int   i;

	list_init(&h);

	for (i = 0; i < 3; i++) {
		d  = scf__malloc(sizeof(Data));

		d->a = i;
		list_add_tail(&h, &d->list);
	}

	list* l;

	for (l = list_head(&h); l != list_sentinel(&h); ) {

		d  = container(l, Data, list);
		l  = list_next(l);

		scf_printf("%d\n", d->a);

		list_del(&d->list);
		scf__free(d);
	}

	return 0;
}

