intptr_t scf_syscall(intptr_t number, ...);

const intptr_t SCF_write = 1;

int scf__write(int fd, uint8_t* buf, uint64_t size)
{
	int ret = scf_syscall(SCF_write, fd, buf, size);

	return ret;
}

