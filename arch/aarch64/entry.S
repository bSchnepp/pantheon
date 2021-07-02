.section ".text.early"


.globl _cpu_start
.globl asm_kern_init_core
.globl CallHVC
.globl CallSMC

_cpu_start:
	/* TODO: Handle spinloop initialization (ala rpi) */
	/* x0 must be preserved and passed to the kernel proper. */

	/* Set up the stack */
	ldr x1, =kern_stack_end
	mov sp, x1
	
	/* x1 and x2 are unused. */
	ldr x1, =bss_begin
	ldr x2, =bss_end
	sub x2, x2, x1
	bl _zero_bss
	
	bl kern_init
	b _hang

_hang:
	b _hang

asm_kern_init_core:
	mov sp, x0
	bl kern_init_core
	b _hang

_zero_bss:
	cbz w2, _zero_bss_end
	str xzr, [x1], #8
	sub w2, w2, #1
	cbnz w2, _zero_bss
	
_zero_bss_end:
	ret

CallSMC:
	smc #0
	ret

CallHVC:
	hvc #0
	ret
