.globl interrupt_table

.include "include.S"

.macro DEFINE_ISR func
.align 7
	kern_entry 1
	mov x0, sp
	bl \func
	b exit_isr
.endm


exit_isr:
	kern_exit 1

.macro DEFINE_ISR_EL0 func
.align 7
	kern_entry 0
	mov x0, sp
	bl \func
	b exit_isr_el0
.endm


exit_isr_el0:
	kern_exit 0


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

.extern write_trap_frame
.extern syscall_table

.section ".text"
.globl enable_interrupts
enable_interrupts:
	msr daifclr, #2
	ret

.globl disable_interrupts
disable_interrupts:
	msr daifset, #2
	ret

.section ".text.exceptions"

spin:
	b spin

.globl _asm_sync_handler_el0
_asm_sync_handler_el0:
	kern_entry 0
	
	mrs x20, esr_el1
	lsr x19, x20, #26	/* To find exception reason, this is needed */
	cmp x19, #0x15
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
	ldr x19, [x25, x26, lsl #3]

	bl enable_interrupts
	mov x0, sp
	bl write_trap_frame
	
	blr x19		/* Actually issue the syscall */

	mov x0, xzr
	bl write_trap_frame
	bl disable_interrupts

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
	DEFINE_ISR_EL0 irq_handler_el0
	DEFINE_ISR_EL0 fiq_handler_el0
	DEFINE_ISR_EL0 err_handler_el0

	DEFINE_ISR_EL0 sync_handler_el0_32
	DEFINE_ISR_EL0 irq_handler_el0_32
	DEFINE_ISR_EL0 fiq_handler_el0_32
	DEFINE_ISR_EL0 err_handler_el0_32

