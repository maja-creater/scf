.text
.global scf__memcpy

scf__memcpy:

	movq  %rdx, %rcx

	testq $0x7, %rdi
	jne   1f

	testq $0x7, %rsi
	jne   1f

	andq  $0x7, %rdx
	shrq  $3,   %rcx

	cld
	rep   movsq

	movq  %rdx, %rcx
1:
	cld
	rep   movsb
	ret
