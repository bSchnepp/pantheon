.section ".bss.early"
.align 16
stack_bottom:
	.skip 1024 * 1024
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

	/* Store the device tree blob pointer */
	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	str x0, [x1]

	/* We also need the address we started at */
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
	
	/* x0 and x1 are free to use now.
	 * Let's use them to compare where we are when clearing the BSS.
	 * There's more efficient ways to write this, but just slamming
	 * one byte at a time is probably efficient enough for now.
	 */
	adrp x0, BSS_AREA
	add x0, x0, :lo12:BSS_AREA
	adrp x1, BSS_END
	add x1, x1, :lo12:BSS_END
	bl _zero_bss

	/* Relocations need to be resolved! 
	 * In theory, the prekernel can now be put pretty much
	 * anywhere in memory: we need to deal with that now.
	 */
	adr x0, _cpu_start
	adr x1, reloc_info	/* .dynamic section */
	ldr x1, [x1, #0x00]
	add x1, x1, x0
	bl ApplyRelocations

go_kern_main:
	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	ldr x0, [x1]

	/* BootInit returns an argument in x0 we'll immediately
	 * pass to the kernel. The kernel's entrypoint is always
	 * this given address in x19, which will need to be changed
	 * later to support KALSR.
	 */
	bl BootInit
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
