.text
.global scf__atomic_inc, scf__atomic_dec, scf__atomic_dec_and_test

scf__atomic_inc:
	lock incl (%rdi)
	ret

scf__atomic_dec:
	lock decl (%rdi)
	ret

scf__atomic_dec_and_test:
	lock   decl (%rdi)
	setz   %al
	movzbl %al, %eax
	ret
.fill 6, 1, 0
