	.text
	.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	
	subq	$32, %rsp
	leaq	.LC1(%rip), %rax
	movq	%rax, %rcx
	call	printf
	addq	$32, %rsp

	subq	$80, %rsp
	call	__main
	movl	$90, 72(%rsp)
	movl	$80, 64(%rsp)
	movl	$7, 56(%rsp)	
	movl	$6, 48(%rsp)	
	movl	$5, 40(%rsp)	
	movl	$4, 32(%rsp) 	
	movl	$3, %r9d 	 	
	movl	$2, %r8d 		
	movl	$1, %edx 		
	leaq	.LC0(%rip), %rax
	movq	%rax, %rcx 		
	call	printf
	addq	$80, %rsp

	popq	%rbp
	ret

.LC0:
	.ascii ">>>%d %d %d %d %d %d %d %d %d<<<\12\0"
.LC1:
	.ascii "Hello\12\0"
