.globl cpu_switch
.globl createprocess_tail
.globl drop_usermode

.extern FinishThread
.extern ReleaseThread

_hang:
	b _hang

createprocess_tail:
	bl FinishThread
	mov x0, x20
	mov x1, x21
	blr x19


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


/* VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP) */
drop_usermode:
	msr elr_el1, x0
	msr spsr_el1, x1
	msr sp_el0, x2
	mov x0, xzr
	mov x1, xzr
	mov x2, xzr
	mov x3, xzr
	mov x4, xzr
	mov x5, xzr
	mov x6, xzr
	mov x7, xzr
	mov x8, xzr
	mov x9, xzr
	mov x10, xzr
	mov x11, xzr
	mov x12, xzr
	mov x13, xzr
	mov x14, xzr
	mov x15, xzr
	mov x16, xzr
	mov x17, xzr
	mov x18, xzr
	mov x19, xzr
	mov x20, xzr
	mov x21, xzr
	mov x22, xzr
	mov x23, xzr
	mov x24, xzr
	mov x25, xzr
	mov x26, xzr
	mov x27, xzr
	mov x28, xzr
	mov x29, xzr
	mov x30, xzr
	eret

