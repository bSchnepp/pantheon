ENTRY(_start)

SECTIONS
{
	/* Start at 4MB */
	. = 4 * 1024 * 1024;

	. = ALIGN(0x1000);
	.text :
	{
		*(.text)
	}
	. = ALIGN(0x1000);

	. = ALIGN(0x1000);
	.rodata :
	{
		*(.rodata*)
	}
	. = ALIGN(0x1000);

	. = ALIGN(0x1000);
	.data :
	{
		*(.data*)
	}
	. = ALIGN(0x1000);

	. = ALIGN(0x1000);
	.bss :
	{
		*(.bss*)
	}
	. = ALIGN(0x1000);

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }	
}
