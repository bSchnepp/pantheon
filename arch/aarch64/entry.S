.section ".text.early"

.globl _cpu_start
.globl asm_kern_init_core
.globl CallHVC
.globl CallSMC

.extern BootInit
.extern kern_init

_cpu_start:
	/* TODO: Handle spinloop initialization (ala rpi) */
	/* x0 must be preserved and passed to the kernel proper. */
	adr x20, .

	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	str x0, [x1]

	adrp x1, initial_addr
	add x1, x1, :lo12:initial_addr
	str x20, [x1]

	/* Set up the stack */
	adrp x0, kern_stack_end
	add x0, x0, :lo12:kern_stack_end
	mov sp, x0

	/* If this is not core/thread 0, dont clear bss again */
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	cbnz x0, go_kern_main
	
	/* x0 and x1 are unused. */
	adrp x0, bss_begin
	add x0, x0, :lo12:bss_begin
	adrp x1, bss_end
	add x1, x1, :lo12:bss_end
	bl _zero_bss

go_kern_main:
	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	ldr x0, [x1]

	adrp x2, initial_addr
	add x2, x2, :lo12:initial_addr
	ldr x1, [x2]

	ldr x2, =_cpu_start
	bl BootInit
	mov x0, x0 /* No-op, but to clarify x0 needs to be preserved. */

	adrp x2, initial_addr
	add x2, x2, :lo12:initial_addr
	ldr x1, [x2]

	ldr x2, =_cpu_start
	bl kern_init
	b _hang

_hang:
	b _hang

asm_kern_init_core:
	mov sp, x0
	bl kern_init_core
	b _hang

_zero_bss:
	cmp x0, x1
	beq _zero_bss_end
	str xzr, [x0]
	add x0, x0, #8
	b _zero_bss
	
_zero_bss_end:
	ret

CallSMC:
	smc #0
	ret

CallHVC:
	hvc #0
	ret

dtb_ptr:
	.xword 0

initial_addr:
	.xword 0
