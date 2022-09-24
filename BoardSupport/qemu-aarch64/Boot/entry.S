.section ".bss.early"
.align 16
stack_bottom:
	.skip 16 * 1024
stack_top:


.section ".text.early"

.globl _cpu_start
.globl asm_kern_init_core
.globl CallHVC
.globl CallSMC

.extern ApplyRelocations
.extern BootInit
.extern kern_init

_cpu_start:
	/* TODO: Handle spinloop initialization (ala rpi) */
	/* x0 must be preserved and passed to the kernel proper. */
	adr x20, .

	/* Make 100% sure interrupts are disabled first. */
	msr daifset, #2

	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	str x0, [x1]

	adrp x1, initial_addr
	add x1, x1, :lo12:initial_addr
	str x20, [x1]

	/* If this is not core/thread 0, go straight to initialization */
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	cbnz x0, go_kern_main

	/* Set up the stack */
	adrp x0, stack_top
	add x0, x0, :lo12:stack_top
	mov sp, x0
	
	/* x0 and x1 are unused. */
	adrp x0, BSS_AREA
	add x0, x0, :lo12:BSS_AREA
	adrp x1, BSS_END
	add x1, x1, :lo12:BSS_END
	bl _zero_bss

	/* Relocations need to be resolved! */
	adr x0, _cpu_start
	adr x1, reloc_info	/* .dynamic section */
	ldr x1, [x1, #0x00]
	add x1, x1, x0
	bl ApplyRelocations

go_kern_main:
	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	ldr x0, [x1]

	bl BootInit
	mov x19, x0 /* save x0 for a little bit */

	/* Jump to the kernel proper */
	mov x0, x19
	ldr x19, =0xFFFFFFFF70000000
	br x19

_hang:
	b _hang

asm_kern_init_core:
	mov sp, x0

	adrp x2, _cpu_start
	add x2, x2, :lo12:_cpu_start
	bl BootInit
	
	/* This should branch to main or init core */
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


.balign 8
reloc_info:
	.quad _DYNAMIC - _cpu_start

/* Must be kept here to guarantee it will not be stripped */
.section ".data.kernel"
.globl kernel_location
kernel_location:
.incbin "pkernel.img"
