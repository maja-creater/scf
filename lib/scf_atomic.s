.text
.global scf__atomic_inc, scf__atomic_dec, scf__atomic_dec_and_test

scf__atomic_inc:
	lock incq (%rdi)
	ret

scf__atomic_dec:
	lock decq (%rdi)
	ret

scf__atomic_dec_and_test:
	lock   decq (%rdi)
	setz   %al
	movzbl %al, %eax
	ret
.fill 3, 1, 0
