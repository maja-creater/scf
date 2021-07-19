
int      scf__brk (uint8_t* addr);
uint8_t* scf__sbrk(uintptr_t inc);

int scf_printf(const char* fmt, ...);

struct scf_mblock_t
{
	uintptr_t     prev;
	uintptr_t     next;
	uintptr_t     size;
};

scf_mblock_t*  scf__mblocks[30];
scf_mblock_t*  scf__free_blocks = NULL;
uint8_t*       scf__last_brk    = NULL;

uint8_t* scf__malloc(uintptr_t size)
{
	scf_mblock_t* b;
	scf_mblock_t* b2;

	uintptr_t bytes   = (sizeof(scf_mblock_t) + size + 63) >> 6 << 6;
	intptr_t  nblocks = sizeof(scf__mblocks) / sizeof(scf__mblocks[0]);

	uintptr_t rest;
	uint8_t*  addr = NULL;
	uint8_t*  p    = NULL;
	intptr_t  i;

	for (i = 0; i < nblocks; i++) {

		if ((64 << i) < bytes)
			continue;

		if (scf__mblocks[i])
			break;
	}

	if (i == nblocks) {

		uintptr_t pages = (bytes + 4095) >> 12;

		p = scf__sbrk(pages << 12);
		if (!p)
			return NULL;
		scf__last_brk = p + (pages << 12);

		rest = (pages << 12) - bytes;

		b = (scf_mblock_t*)p;

		b->prev = 0x3;
		b->next = bytes;

	} else {
		b = scf__mblocks[i];
		scf__mblocks[i] = (scf_mblock_t*)b->size;

		uintptr_t bytes2;
		if (b->prev & 0x2)
			bytes2 = b->next;
		else
			bytes2 = b->next - (uintptr_t)b;

		p = (uint8_t*)b;

		rest = bytes2 - bytes;
	}

	addr = p + sizeof(scf_mblock_t);

	if (0 == rest) {
		scf_printf("74, b: %p, scf__last_brk: %#lx\n", b, scf__last_brk);
		return addr;
	}

	p  += bytes;
	b2  = (scf_mblock_t*)p;

	for (; i >= 0; i--) {

		if (rest >= (64 << i)) {
			b2->size = (uintptr_t)scf__mblocks[i];
			scf__mblocks[i] = b2;
			break;
		}
	}

	b ->next = (uintptr_t)b2;
	b2->prev = (uintptr_t)b;

	if (b->prev & 0x2) {
		b2->prev |=  0x2;
		b2->prev &= ~0x4;
		b ->prev &= ~0x2;
		b2->next  =  rest;
	}

	scf_printf("100, b: %p, scf__last_brk: %#lx\n", b, scf__last_brk);
	return addr;
}

int scf__free(uint8_t* p)
{
	p -= sizeof(scf_mblock_t);

	scf_mblock_t*  b = (scf_mblock_t*)p;
	scf_mblock_t*  b2;
	scf_mblock_t*  b3;
	scf_mblock_t** pb;

	uintptr_t bytes;
	if (b->prev & 0x2)
		bytes = b->next;
	else
		bytes = b->next - (uintptr_t)b;

	intptr_t  nblocks = sizeof(scf__mblocks) / sizeof(scf__mblocks[0]);
	intptr_t  i;

	for (i = nblocks - 1; i >= 0; i--) {

		if (bytes >= (64 << i))
			break;
	}

	while (!(b->prev & 0x2)) {

		b2 = (scf_mblock_t*)b->next;

		uintptr_t bytes2;
		if (b2->prev & 0x2)
			bytes2 = b2->next;
		else
			bytes2 = b2->next - (uintptr_t)b2;

		for (i = nblocks - 1; i >= 0; i--) {

			if (bytes2 >= (64 << i))
				break;
		}

		for (pb = &scf__mblocks[i]; *pb; pb = (scf_mblock_t**)&(*pb)->size) {

			if (*pb == b2)
				break;
		}
		if (!*pb)
			break;

		*pb = (scf_mblock_t*)b2->size;

		bytes += bytes2;

		if (b2->prev &  0x2) {
			b->prev |= 0x2;
			b->next = bytes;
		} else {
			b3       = (scf_mblock_t*)b2->next;
			b->next  = b2->next;
			b3->prev = (uintptr_t)b | (b3->prev & 0x3);
		}
	}

	while (!(b->prev & 0x1)) {

		b2 = (scf_mblock_t*)(b->prev & ~0x7);

		uintptr_t bytes2 = (uintptr_t)b - (uintptr_t)b2;

		for (i = nblocks - 1; i >= 0; i--) {

			if (bytes2 >= (64 << i))
				break;
		}

		for (pb = &scf__mblocks[i]; *pb; pb = (scf_mblock_t**)&(*pb)->size) {
			if (*pb == b2)
				break;
		}
		if (!*pb)
			break;

		*pb = (scf_mblock_t*)b2->size;

		bytes += bytes2;

		if (b->prev & 0x2) {
			b2->prev |= 0x2;
			b2->next  = bytes;

		} else {
			b3       = (scf_mblock_t*)b->next;
			b2->next = b->next;
			b3->prev = (uintptr_t)b2 | (b3->prev & 0x3);
		}

		b = b2;
	}

	if (b->prev & 0x2)
		bytes = b->next;
	else
		bytes = b->next - (uintptr_t)b;

	if (0x3 == (b->prev) & 0x3) {

		if (scf__last_brk == (uint8_t*)b + bytes) {

			scf_printf("211, b: %p, scf__last_brk: %#lx\n", b, scf__last_brk);

			scf__last_brk = (uint8_t*)b;
			scf__brk((uint8_t*)b);

			int flag = 1;
			while (flag) {
				flag = 0;

				pb = &scf__free_blocks;
				while (*pb) {

					b = *pb;
					bytes = b->next;

					if (scf__last_brk != (uint8_t*)b + bytes) {

						pb = (scf_mblock_t**)&b->size;
						continue;
					}
					*pb = (scf_mblock_t*)b->size;

					scf_printf("232, b: %p, scf__last_brk: %#lx\n", b, scf__last_brk);

					scf__last_brk = (uint8_t*)b;
					scf__brk((uint8_t*)b);
					flag = 1;
				}
			}
		} else {
			b->size = (uintptr_t)scf__free_blocks;
			scf__free_blocks = b;
			scf_printf("242, b: %p, scf__last_brk: %#lx\n", b, scf__last_brk);
		}
		return 0;
	}

	for (i = nblocks - 1; i >= 0; i--) {
		if (bytes >= (64 << i))
			break;
	}

	b->size = (uintptr_t)scf__mblocks[i];
	scf__mblocks[i] = b;

	scf_printf("255, b: %p\n", b);
	return 0;
}

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

