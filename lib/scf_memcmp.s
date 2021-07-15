.text
.global scf__memcmp, scf__memcmpi, scf__memcmpq

scf__memcmp:

	movq  %rdx, %rcx
	rep   cmpsb

	jl    1f
	jg    2f

	xorq  %rax, %rax
	ret
1:
	movq  $-1,  %rax
	ret
2:
	movq  $1,   %rax
	ret

scf__memcmpi:

	movq  %rdx, %rcx
	rep   cmpsd

	jl    1f
	jg    2f

	xorq  %rax, %rax
	ret
1:
	movq  $-1,  %rax
	ret
2:
	movq  $1,   %rax
	ret

scf__memcmpq:

	movq  %rdx, %rcx
	rep   cmpsq

	jl    1f
	jg    2f

	xorq  %rax, %rax
	ret
1:
	movq  $-1,  %rax
	ret
2:
	movq  $1,   %rax
	ret

