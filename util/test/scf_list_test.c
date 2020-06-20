#include"scf_list.h"

typedef struct {
	scf_list_t	l;
	int			d;
} d_t;

int main()
{
	scf_list_t h;
	scf_list_init(&h);

	int i;
	for (i = 0; i < 10; i++) {
		d_t* d = malloc(sizeof(d_t));
		assert(d);
		d->d = i;
//		scf_list_add_tail(&h, &d->l);
		scf_list_add_front(&h, &d->l);
	}

	scf_list_t* l;
	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); l = scf_list_next(l)) {
		d_t* d = scf_list_data(l, d_t, l);
		printf("%d\n", d->d);
	}

	while (!scf_list_empty(&h)) {
		scf_list_t* l = scf_list_head(&h);

		d_t* d = scf_list_data(l, d_t, l);

		scf_list_del(&d->l);
		free(d);
		d = NULL;
	}

	return 0;
}

