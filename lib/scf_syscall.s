.text
.global scf_syscall

scf_syscall:

	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %rdx
	movq %r8,  %r10
	movq %r9,  %r8
	movq 8(%rsp), %r9

	syscall
	ret
.fill 6, 1, 0
