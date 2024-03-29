/* Metadata has to be placed into this file, 
 * to ensure objdump doesn't strip it. KEEP()
 * doesn't appear to guarantee it is kept.
 */

/* The initial area here will be used for bringup of the kernel. */
.section ".text.metadata"
.globl metadata_table

/* This area will likely be interpretted as code: make sure the
 * first instruction is to jump to the kernel proper.
 */
metadata_table:
b run_kernel
.ascii 	"PANTHEON"			/* Magic string. */
.word 	TEXT_AREA - metadata_table	/* Where is the offset of text? */
.word 	TEXT_END - metadata_table	/* What is the end of text? */
.word 	RODATA_AREA - metadata_table	/* Where is the offset of rodata? */
.word 	RODATA_END - metadata_table	/* What is the end of rodata? */
.word 	DATA_AREA - metadata_table	/* Where is the offset of data? */
.word 	DATA_END - metadata_table	/* What is the end of data? */
.word 	BSS_AREA - metadata_table	/* Where is the offset of bss? */
.word 	BSS_END - metadata_table	/* What is the end of bss? */
.word 	INIT_ARRAY_AREA - metadata_table	/* Where is the offset of init_array? */
.word 	INIT_ARRAY_END - metadata_table		/* What is the end of init_array? */
.quad	_DYNAMIC - metadata_table		/* The location of the relocation info */

.extern prepare_kernel_stack
run_kernel:
	/* Save x0 in x19: we don't need any of the general purpose
	 * regs, but will need to use x0 for setting up the stack.
	 */
	mov x19, x0

	/* The return value needs to be our kernel stack. */
	bl prepare_kernel_stack
	mov sp, x0 

	/* Restore x0, since that is the initial boot info. */
	mov x0, x19
	b kern_init
	bl .

.section ".data"

.globl sysm_location
sysm_location:
.incbin "sysm"

.globl prgm_location
prgm_location:
.incbin "prgm"

