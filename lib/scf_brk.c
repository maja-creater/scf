intptr_t scf_syscall(intptr_t number, ...);

const intptr_t SCF_brk      = 12;

uintptr_t      scf__cur_brk = 0;

int scf__brk(uint8_t* addr)
{
	scf__cur_brk = scf_syscall(SCF_brk, addr);

	if (scf__cur_brk == addr)
		return 0;
	return -1;
}

uint8_t* scf__sbrk(uintptr_t inc)
{
	if (0 == scf__cur_brk)
		scf__cur_brk = scf_syscall(SCF_brk, 0);

	uintptr_t addr = scf__cur_brk;

	if (addr + inc < scf__cur_brk)
		return NULL;

	scf__cur_brk = scf_syscall(SCF_brk, addr + inc);

	if (addr < scf__cur_brk)
		return addr;
	return NULL;
}


