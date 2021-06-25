.globl cpu_switch
.globl createprocess_tail

.extern enable_interrupts

_hang:
	b _hang

createprocess_tail:
	bl enable_interrupts
	mov x0, x20
	blr x19
	b _hang	/* Should never reach */


/* void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset); */
cpu_switch:
	/* Ensure x8 holds the offset where Regs is for Old */
	add x8, x0, x2
	
	/* save callee registers */
	mov x9, sp
	stp x19, x20, [x8], #16
	stp x21, x22, [x8], #16
	stp x23, x24, [x8], #16
	stp x25, x26, [x8], #16
	stp x27, x28, [x8], #16
	stp x29, x9, [x8], #16
	str x30, [x8]

	/* x8 now holds the offset where regs is in New */
	add x8, x1, x2
	
	/* restore callee registers */
	ldp x19, x20, [x8], #16
	ldp x21, x22, [x8], #16
	ldp x23, x24, [x8], #16
	ldp x25, x26, [x8], #16
	ldp x27, x28, [x8], #16
	ldp x29, x9, [x8], #16
	ldr x30, [x8]
	mov sp, x9
	ret
