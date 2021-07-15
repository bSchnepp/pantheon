.section ".text.early"

.globl _cpu_start
.globl asm_kern_init_core
.globl CallHVC
.globl CallSMC

.extern kern_init
.extern initial_paging

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
	
	/* x0 and x1 are unused. */
	adrp x0, bss_begin
	add x0, x0, :lo12:bss_begin
	adrp x1, bss_end
	add x1, x1, :lo12:bss_end

	sub x1, x1, x0
	bl _zero_bss

	adrp x1, dtb_ptr
	add x1, x1, :lo12:dtb_ptr
	ldr x0, [x1]

	adrp x2, initial_addr
	add x1, x1, :lo12:initial_addr
	ldr x1, [x2]

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

dtb_ptr:
	.xword 0

initial_addr:
	.xword 0
