ENTRY(_cpu_start)

SECTIONS
{
	/* aarch64 load address */
	. = 0x80000 + 0x40000000;

	.text :
	{
		*(.text.early)
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
	bss_size = bss_end - bss_begin;

	/* Hack for now to make this work for us. */
	kern_stack_begin = .;
	.kern_stack :
	{
		. += 16384;
	}
	kern_stack_end = .;
	kern_stack_size = kern_stack_end - kern_stack_begin;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
	
}
