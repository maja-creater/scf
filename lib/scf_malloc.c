
int      scf__brk (uint8_t* addr);
uint8_t* scf__sbrk(uintptr_t inc);

int      scf_printf(const char* fmt, ...);

int      scf__memseti(uint8_t* dst, uint32_t i,   uintptr_t n);
int      scf__memsetq(uint8_t* dst, uint64_t q,   uintptr_t n);
int      scf__memcpy (uint8_t* dst, uint8_t* src, uintptr_t size);

// min size of block is 64, so low 6 bits can be used for flags
// flags in prev_size:
//       Free Last First
//       2    1    0
struct scf_mblock_t
{
	uintptr_t     magic;
	uintptr_t     prev_size;
	uintptr_t     cur_size;
	uintptr_t     free;
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

		b->prev_size = 0x3;
		b->cur_size  = bytes;
		b->magic     = 0x10f0;
	} else {
		b = scf__mblocks[i];
		scf__mblocks[i] = (scf_mblock_t*)b->free;

		p = (uint8_t*)b;

		rest = b->cur_size - bytes;

		b->prev_size &= ~0x4;
		b->magic      = 0x10f0;
	}

	addr = p + sizeof(scf_mblock_t);

	if (0 == rest) {
		scf_printf("%s(),%d, b: %p, scf__last_brk: %p\n", __func__, __LINE__, b, scf__last_brk);
		return addr;
	}

	p  += bytes;
	b2  = (scf_mblock_t*)p;

	for (; i >= 0; i--) {

		if (rest >= (64 << i)) {
			b2->free = (uintptr_t)scf__mblocks[i];
			scf__mblocks[i] = b2;
			break;
		}
	}

	b ->cur_size  = bytes;
	b2->cur_size  = rest;
	b2->prev_size = bytes | 0x4;
	b2->magic     = 0xf010;

	if (b->prev_size & 0x2) {
		b->prev_size &= ~0x2;
		b2->prev_size |= 0x2;
	}

	scf_printf("%s(),%d, b: %p, scf__last_brk: %p\n", __func__, __LINE__, b, scf__last_brk);
	return addr;
}

int scf__free(uint8_t* p)
{
	p -= sizeof(scf_mblock_t);

	scf_mblock_t*  b = (scf_mblock_t*)p;
	scf_mblock_t*  b2;
	scf_mblock_t*  b3;
	scf_mblock_t** pb;

	uintptr_t bytes   = b->cur_size;
	intptr_t  nblocks = sizeof(scf__mblocks) / sizeof(scf__mblocks[0]);
	intptr_t  i;

	if (b->prev_size & 0x4) {
		scf_printf("%s(), %d, error: double free: %p\n", __func__, __LINE__, p);
		return -1;
	}

	if (0x10f0 != b->magic) {
		scf_printf("%s(), %d, error: corruption free: %p\n", __func__, __LINE__, p);
		return -1;
	}

	b->prev_size |= 0x4;
	b->magic      = 0xf010;

	while (!(b->prev_size & 0x2)) {

		b2 = (scf_mblock_t*)((uintptr_t)b + bytes);

		uintptr_t bytes2 = b2->cur_size;

		for (i = nblocks - 1; i >= 0; i--) {

			if (bytes2 >= (64 << i))
				break;
		}

		for (pb = &scf__mblocks[i]; *pb; pb = (scf_mblock_t**)&(*pb)->free) {

			if (*pb == b2)
				break;
		}
		if (!*pb)
			break;

		*pb = (scf_mblock_t*)b2->free;

		bytes       += bytes2;
		b->cur_size  = bytes;

		if (b2->prev_size & 0x2)
			b->prev_size |= 0x2;
		else {
			b3 = (scf_mblock_t*)((uintptr_t)b2 + bytes2);

			b3->prev_size = bytes | (b3->prev_size & 0x7);
		}
	}

	while (!(b->prev_size & 0x1)) {

		uintptr_t bytes2 = b->prev_size & ~0x7;

		b2 = (scf_mblock_t*)((uintptr_t)b - bytes2);

		bytes2 = b2->cur_size;

		for (i = nblocks - 1; i >= 0; i--) {

			if (bytes2 >= (64 << i))
				break;
		}

		for (pb = &scf__mblocks[i]; *pb; pb = (scf_mblock_t**)&(*pb)->free) {
			if (*pb == b2)
				break;
		}
		if (!*pb)
			break;

		*pb = (scf_mblock_t*)b2->free;

		bytes        += bytes2;
		b2->cur_size  = bytes;

		if (b->prev_size  &  0x2)
			b2->prev_size |= 0x2;
		else {
			b3 = (scf_mblock_t*)((uintptr_t)b2 + bytes);

			b3->prev_size = bytes | (b3->prev_size & 0x7);
		}

		b = b2;
	}

	bytes = b->cur_size;

	if (0x7 == (b->prev_size) & 0x7) {

		if (scf__last_brk == (uint8_t*)b + bytes) {

			scf_printf("%s(), %d, b: %p, scf__last_brk: %p\n", __func__, __LINE__, b, scf__last_brk);
			scf__last_brk = (uint8_t*)b;
			scf__brk((uint8_t*)b);

			int flag = 1;
			while (flag) {
				flag = 0;

				pb = &scf__free_blocks;
				while (*pb) {

					b = *pb;
					bytes = b->cur_size;

					if (scf__last_brk != (uint8_t*)b + bytes) {

						pb = (scf_mblock_t**)&b->free;
						continue;
					}
					*pb = (scf_mblock_t*)b->free;

					scf_printf("%s(), %d, b: %p, scf__last_brk: %p\n", __func__, __LINE__, b, scf__last_brk);
					scf__last_brk = (uint8_t*)b;
					scf__brk((uint8_t*)b);
					flag = 1;
				}
			}
		} else {
			b->free = (uintptr_t)scf__free_blocks;
			scf__free_blocks = b;
			scf_printf("%s(), %d, b: %p\n", __func__, __LINE__, b);
		}
		return 0;
	}

	for (i = nblocks - 1; i >= 0; i--) {
		if (bytes >= (64 << i))
			break;
	}

	b->free = (uintptr_t)scf__mblocks[i];
	scf__mblocks[i] = b;

	scf_printf("%s(), %d, b: %p\n", __func__, __LINE__, b);
	return 0;
}

uint8_t* scf__calloc(uintptr_t n, uintptr_t size)
{
	scf_mblock_t*  b;
	uintptr_t      bytes;
	uint8_t*       p;

	size *= n;

	p = scf__malloc(size);
	if (!p)
		return NULL;

	b = (scf_mblock_t*)(p - sizeof(scf_mblock_t));

	bytes  = b->cur_size;
	bytes -= sizeof(scf_mblock_t);
	scf_printf("%s(),%d, calloc, b: %p, bytes: %ld\n", __func__, __LINE__, b, bytes);

	scf__memsetq(p, 0, bytes >> 3);

	return p;
}

uint8_t* scf__realloc(uint8_t* p, uintptr_t size)
{
	scf_mblock_t*  b;
	scf_mblock_t*  b2;
	scf_mblock_t*  b3;
	uintptr_t      bytes;
	uint8_t*       p2;

	b = (scf_mblock_t*)(p - sizeof(scf_mblock_t));

	bytes  = b->cur_size;
	bytes -= sizeof(scf_mblock_t);

	if (bytes < size) {
		p2 = scf__malloc(size);
		if (!p2)
			return NULL;

		scf__memcpy(p2, p, bytes);
		scf__free(p);

		scf_printf("%s(), %d, realloc: %p->%p, size: %ld\n", __func__, __LINE__, p, p2, size);
		return p2;
	}

	size   = (sizeof(scf_mblock_t) + 63 + size) >> 6 << 6;
	bytes +=  sizeof(scf_mblock_t);

	if (bytes < size + 64)
		return p;

	b2 = (scf_mblock_t*)((uintptr_t)b + size);

	b ->cur_size = size;
	b2->cur_size = bytes - size;

	if (b->prev_size & 0x2) {
		b->prev_size &= ~0x2;
		b2->prev_size = size | 0x2;
	} else {
		b3 = (scf_mblock_t*)((uintptr_t)b + bytes);

		b2->prev_size = size;
		b3->prev_size = (bytes - size) | (b3->prev_size & 0x7);
	}

	b2->magic = 0x10f0;

	p2 = (uint8_t*)b2 + sizeof(scf_mblock_t);
	scf__free(p2);

	scf_printf("%s(), %d, realloc: %p, free b2: %p, size: %ld\n", __func__, __LINE__, p, p2, size);
	return p;
}

