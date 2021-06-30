.globl interrupt_table

.equ SYSCALL_AMT, 4

.macro kern_entry, dpl
	/* 31 regs, plus sp, pc, and pstate  */
	sub sp, sp, #((31 * 8) + (3 * 8))
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
	
	/* Use x19, x20, and x21 for our use.
	 * The necessary pc for going back to user code is
	 * first, then the program status register, and finally the stack
	 * pointer.
	 */
	mrs x19, elr_el1
	mrs x20, spsr_el1
	.if \dpl == 1
	add x21, sp, ((32 * 8) + (2 * 8))
	.else
	mrs x21, sp_el0
	.endif

	/* Store the status registers (and x30)... */
	stp x30, x19, [sp, #15 * 16]
	stp x20, x21, [sp, #16 * 16]
.endm

.macro kern_exit, dpl
	/* Registers should be unwound backwards from the order saved to. */
	ldp x20, x21, [sp, #16 * 16]
	ldp x30, x19, [sp, #15 * 16]

	.if \dpl != 1
	msr sp_el0, x19
	.endif
	msr spsr_el1, x20
	msr elr_el1, x19

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
	add sp, sp, #((32 * 8) + (2 * 8))
	eret
.endm

.macro DEFINE_ISR func
.align 7
	kern_entry 1
	bl \func
	b exit_isr
.endm


exit_isr:
	kern_exit 1


.extern sync_handler_el1_sp0
.extern err_handler_el1_sp0
.extern fiq_handler_el1_sp0
.extern irq_handler_el1_sp0

.extern sync_handler_el1
.extern err_handler_el1
.extern fiq_handler_el1
.extern irq_handler_el1

.extern err_handler_el0
.extern fiq_handler_el0
.extern irq_handler_el0

.extern sync_handler_el0_32
.extern err_handler_el0_32
.extern fiq_handler_el0_32
.extern irq_handler_el0_32

.extern syscall_table

.section ".text.exceptions"


spin:
	b spin

.extern enable_interrupts
.globl _asm_sync_handler_el0
_asm_sync_handler_el0:
	kern_entry 0
	mrs x25, esr_el1
	lsr x25, x25, #26	/* To find exception reason, this is needed */
	cmp x25, #0x15
	bne _sync_handler_el0_exit	/* Exit if that's not right */
	adr x25, syscall_table	/* x25 holds the syscall table pointer */
	uxtw x26, w8		/* unsigned extend to word for x26 with syscall no */
	mov x27, #SYSCALL_AMT	/* Is this number valid? */
	cmp x26, x27		/* Is this syscall number valid? */
	bhs _sync_handler_el0_exit	/* If not, exit again */

	/* Load the syscall pointer. 
	 * Since sizeof(UINT_PTR) == 8, the offset used needs to be at
	 * the syscall table + (8 * index), which is the same as lsl 3.
	 */
	ldr x9, [x25, x26, lsl #3]
	bl enable_interrupts
	blr x9
	bl disable_interrupts

	/* x0 is the top of the stack. x1-x8 should also be sent maybe? */
	str x0, [sp, #(0 * 8)]
_sync_handler_el0_exit:
	kern_exit 0


interrupt_table:
	DEFINE_ISR sync_handler_el1_sp0
	DEFINE_ISR irq_handler_el1_sp0
	DEFINE_ISR fiq_handler_el1_sp0
	DEFINE_ISR err_handler_el1_sp0

	DEFINE_ISR sync_handler_el1
	DEFINE_ISR irq_handler_el1
	DEFINE_ISR fiq_handler_el1
	DEFINE_ISR err_handler_el1

	.align 7 
	b _asm_sync_handler_el0
	DEFINE_ISR irq_handler_el0
	DEFINE_ISR fiq_handler_el0
	DEFINE_ISR err_handler_el0

	DEFINE_ISR sync_handler_el0_32
	DEFINE_ISR irq_handler_el0_32
	DEFINE_ISR fiq_handler_el0_32
	DEFINE_ISR err_handler_el0_32

