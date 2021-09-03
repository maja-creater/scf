.text
.global scf_syscall, scf__open

scf_syscall:

	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %rdx
	movq %r8,  %r10
	movq %r9,  %r8
	movq 8(%rsp), %r9

#	movq  8(%rsp), %r10
#	movq 16(%rsp), %r8
#	movq 24(%rsp), %r9

	syscall
	ret

scf__open:
	movq %rdx, %rcx
	movq %rsi, %rdx
	movq %rdi, %rsi
	movq $2,   %rdi
	call scf_syscall
	ret
.fill 0, 1, 0
