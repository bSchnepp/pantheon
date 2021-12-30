
# Until we do ASLR, this is static.
.section .bss
.global stack_top
stack_top:
	.align 0x16
	.skip 32 * 1024


.section .text

.extern main
.extern svc_ExitProcess

.global _start
_start:
	ldr x0, =stack_top
	mov sp, x0

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

	bl main 		// Run actual user code
	bl svc_ExitProcess	// Terminate the process if we ever come back
	b .			// Give up and loop forever if that didnt work
