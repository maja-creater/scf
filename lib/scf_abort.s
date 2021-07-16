.text
.global scf__abort

scf__abort:
	xorq %rax,  %rax
	mov  %rax, (%rax)
	ret
.fill 1, 1, 0
