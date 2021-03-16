.section ".text.early"
.globl _cpu_start

_cpu_start:
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

_zero_bss:
	cbz w2, _zero_bss_end
	str xzr, [x1], #8
	sub w2, w2, #1
	cbnz w2, _zero_bss
	
_zero_bss_end:
	ret
