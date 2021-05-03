.text
.global __asm_co_task_run, __asm_co_task_yield, __asm_co_task_resume
.global __jmp_ret, __jmp_yield

.align 8
__asm_co_task_run:
# rdi, task_rsp addr
# rsi, stack_data
# rdx, task_rip
# rcx, stack_len
# r8,  task_rsp0 addr 

	push %rbp
	push %rdi
	push %rsi

	push %rbx
	push %rcx
	push %rdx

	push %r12
	push %r13
	push %r14
	push %r15

	mov (%r8), %rax
	mov  %rsp, (%r8)
	mov  %rdx, %r8

	mov  %rsp, %rbp

_fill:
	sub  %rcx, %rsp
	mov  %rsp, %rdi
	cld
	rep movsb

	test %rax, %rax
	jne  _jmp

	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx

	call *%r8
	jmp  _reset

_jmp:
	jmp  *%r8

_reset:
	mov %rbp, %rsp
	mov %rsp, %rax

__jmp_ret:
	pop %r15
	pop %r14
	pop %r13
	pop %r12

	pop %rdx
	pop %rcx
	pop %rbx

	pop %rsi
	pop %rdi
	pop %rbp
	ret

.align 8
__asm_co_task_yield:
# rdi, task pointer
# rsi, task_rip  addr
# rdx, task_rsp  addr
# rcx, task_rsp0

	push %rbp
	push %rdi
	push %rsi

	push %rbx
	push %rcx
	push %rdx

	push %r12
	push %r13
	push %r14
	push %r15

	movq $__jmp_yield, (%rsi)
	mov  %rsp, (%rdx)

	push %rsp
	push %rcx

	call __save_stack

	pop  %rcx
	pop  %rsp
	mov  %rsp, %rax
	mov  %rcx, %rsp

	jmp  __jmp_ret

__jmp_yield:
	pop %r15
	pop %r14
	pop %r13
	pop %r12

	pop %rdx
	pop %rcx
	pop %rbx

	pop %rsi
	pop %rdi
	pop %rbp

	mov %rsp, %rax
	ret

