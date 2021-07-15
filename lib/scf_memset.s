.text
.global scf__memset, scf__memseti, scf__memsetq

scf__memset:
	movq  %rdx, %rcx
	movq  %rsi, %rax

	cld
	rep   stosb
	ret

scf__memseti:
	movq  %rdx, %rcx
	movq  %rsi, %rax

	cld
	rep   stosl
	ret

scf__memsetq:
	movq  %rdx, %rcx
	movq  %rsi, %rax

	cld
	rep   stosq
	ret
.fill 1, 1, 0
