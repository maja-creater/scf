
uint8_t* scf__malloc(uintptr_t size);
int      scf__free(uint8_t* p);

int main()
{
	uint8_t* p0 = scf__malloc(1000);
	uint8_t* p1 = scf__malloc(1320);
	uint8_t* p2 = scf__malloc(2510);
	uint8_t* p3 = scf__malloc(4510);
	uint8_t* p4 = scf__malloc(510);
	uint8_t* p5 = scf__malloc(6510);
	uint8_t* p6 = scf__malloc(510);
	uint8_t* p7 = scf__malloc(11510);

	scf__free(p0);

	*p1 = 1;
	scf__free(p1);

	*p2 = 2;
	*p4 = 4;

	scf__free(p4);
	scf__free(p5);
	scf__free(p2);
	scf__free(p6);
	scf__free(p3);

	scf__free(p7);
	return 0;
}

