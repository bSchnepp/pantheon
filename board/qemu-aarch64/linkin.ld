ENTRY(_cpu_start)

SECTIONS
{
	/* aarch64 load address */
	. = 0x80000 + 0x40000000;

	. = ALIGN(64 * 1024);
	TEXT_AREA = .;
	kern_begin = .;
	.text.early :
	{
		*(.text.early)
	}

	. = ALIGN(4096);
	. = ALIGN(0x800);
	.text.exceptions :
	{
		*(.text.exceptions)
	}
	. = ALIGN(4096);

	. = ALIGN(4096);
	.text :
	{
		*(.text)
	}
	. = ALIGN(64 * 1024);


	. = ALIGN(64 * 1024);
	RODATA_AREA = .;
	.rodata :
	{
		*(.rodata*)
	}
	. = ALIGN(64 * 1024);

	. = ALIGN(64 * 1024);
	DATA_AREA = .;
	.data :
	{
		*(.data*)
	}
	. = ALIGN(64 * 1024);

	. = ALIGN(64 * 1024);
	BSS_AREA = .;
	bss_begin = .;
	.bss :
	{
		*(.bss.early)
		*(.bss*)
	}

	/* Hack for now to make this work for us. */
	kern_stack_begin = .;
	.kern_stack :
	{
		. += 2 * 1024 * 1024;
	}
	kern_stack_end = .;
	kern_stack_size = kern_stack_end - kern_stack_begin;

	. = ALIGN(4096);
	TTBR0_AREA = .;
	. += 4096;
	TTBR1_AREA = .;
	. += 4096;
	TTL2_AREA = .;
	. += 4096;
	TTL3_AREA = .;
	. += 4096;

	. = ALIGN(64 * 1024);
	bss_end = .;
	bss_size = bss_end - bss_begin;

	. += 0x1000;	/* Add extra space just in case... */
	. = ALIGN(64 * 1024);
	kern_end = .;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
	
}
