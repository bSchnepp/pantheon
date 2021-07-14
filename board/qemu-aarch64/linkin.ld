ENTRY(_cpu_start)

SECTIONS
{
	/* aarch64 load address */
	. = 0x80000 + 0x40000000;

	. = ALIGN(4096);
	kern_begin = .;

	.text.early :
	{
		*(.text.early)
	}

	. = ALIGN(0x800);
	.text.exceptions :
	{
		*(.text.exceptions)
	}

	. = ALIGN(4096);
	.text :
	{
		*(.text)
	}

	.rodata :
	{
		*(.rodata*)
	}
	
	.data :
	{
		*(.data*)
	}
	
	bss_begin = .;
	.bss :
	{
		*(.bss.early)
		*(.bss*)
	}
	bss_end = .;

	. = ALIGN(4096);
	TTBR0_AREA = .;
	. += 4096;
	TTBR1_AREA = .;
	. += 4096;
	TTL2_AREA = .;
	. += 4096;
	TTL3_AREA = .;
	. += 4096;
	bss_size = bss_end - bss_begin;

	/* Hack for now to make this work for us. */
	kern_stack_begin = .;
	.kern_stack :
	{
		. += 2 * 1024 * 1024;
	}
	kern_stack_end = .;
	kern_stack_size = kern_stack_end - kern_stack_begin;


	. = ALIGN(4096);
	kern_end = .;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
	
}
