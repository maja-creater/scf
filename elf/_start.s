.text
.global _start, main

_start:
	call main
	mov  %rax, %rdi
	mov  $60,  %rax
	syscall
