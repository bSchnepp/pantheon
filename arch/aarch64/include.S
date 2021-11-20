.equ SYSCALL_AMT, 10
.equ TRAP_SZ, (16*17)

.macro kern_entry, dpl
	/* 31 regs, plus sp, pc, and pstate  */
	sub sp, sp, #TRAP_SZ
	stp x0, x1, [sp, #0 * 16]
	stp x2, x3, [sp, #1 * 16]
	stp x4, x5, [sp, #2 * 16]
	stp x6, x7, [sp, #3 * 16]
	stp x8, x9, [sp, #4 * 16]
	stp x10, x11, [sp, #5 * 16]
	stp x12, x13, [sp, #6 * 16]
	stp x14, x15, [sp, #7 * 16]
	stp x16, x17, [sp, #8 * 16]
	stp x18, x19, [sp, #9 * 16]
	stp x20, x21, [sp, #10 * 16]
	stp x22, x23, [sp, #11 * 16]
	stp x24, x25, [sp, #12 * 16]
	stp x26, x27, [sp, #13 * 16]
	stp x28, x29, [sp, #14 * 16]
	
	/* Use x21, x22, and x23 for our use.
	 * The necessary pc for going back to user code is
	 * first, then the program status register, and finally the stack
	 * pointer.
	 */
	mrs x21, elr_el1
	mrs x22, spsr_el1
	.if \dpl == 1
	add x23, sp, #TRAP_SZ
	.else
	mrs x23, sp_el0
	.endif

	/* Store the status registers (and x30)... */
	stp x30, x21, [sp, #15 * 16]
	stp x22, x23, [sp, #16 * 16]
.endm

.macro kern_exit, dpl
	/* Registers should be unwound backwards from the order saved to. */
	ldp x22, x23, [sp, #16 * 16]
	ldp x30, x21, [sp, #15 * 16]

	.if \dpl != 1
	msr sp_el0, x23
	.endif
	msr spsr_el1, x22
	msr elr_el1, x21

	ldp x28, x29, [sp, #14 * 16]
	ldp x26, x27, [sp, #13 * 16]
	ldp x24, x25, [sp, #12 * 16]
	ldp x22, x23, [sp, #11 * 16]
	ldp x20, x21, [sp, #10 * 16]
	ldp x18, x19, [sp, #9 * 16]
	ldp x16, x17, [sp, #8 * 16]
	ldp x14, x15, [sp, #7 * 16]
	ldp x12, x13, [sp, #6 * 16]
	ldp x10, x11, [sp, #5 * 16]
	ldp x8, x9, [sp, #4 * 16]
	ldp x6, x7, [sp, #3 * 16]
	ldp x4, x5, [sp, #2 * 16]
	ldp x2, x3, [sp, #1 * 16]
	ldp x0, x1, [sp, #0 * 16]
	add sp, sp, #TRAP_SZ
	eret
.endm

