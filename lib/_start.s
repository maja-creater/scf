.text
.global _start, main

_start:
	call main
	mov  %rax, %rdi
	mov  $60,  %rax
	syscall
.fill 7, 1, 0
