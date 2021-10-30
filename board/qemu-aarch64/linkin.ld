ENTRY(_cpu_start)

SECTIONS
{
	/* aarch64 load address */
	. = 0x80000 + 0x40000000;

	. = ALIGN(0x1000);
	TEXT_AREA = .;
	kern_begin = .;
	.text.early :
	{
		*(.text.early)
	}

	. = ALIGN(0x1000);
	. = ALIGN(0x800);
	.text.exceptions :
	{
		*(.text.exceptions)
	}
	. = ALIGN(0x1000);

	. = ALIGN(0x1000);
	.text :
	{
		*(.text)
	}
	. = ALIGN(0x1000);
	TEXT_END = .;


	. = ALIGN(0x1000);
	RODATA_AREA = .;
	.rodata :
	{
		*(.rodata*)
	}
	. = ALIGN(0x1000);
	RODATA_END = .;

	. = ALIGN(0x1000);
	DATA_AREA = .;
	.data :
	{
		*(.data*)
	}
	. = ALIGN(0x1000);
	DATA_END = .;

	. = ALIGN(0x1000);
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

	. = ALIGN(0x1000);
	bss_end = .;
	bss_size = bss_end - bss_begin;

	. += 0x1000;	/* Add extra space just in case... */
	. = ALIGN(64 * 1024);
	kern_end = .;
	BSS_END = .;

	USER_BEGIN = .;
	.user.text : 
	{
		build/Userland* (.text)
	}

	.user.rodata : 
	{
		build/Userland* (.rodata)
	}

	.user.data : 
	{
		build/Userland* (.data)
	}

	.user.bss : 
	{
		build/Userland* (.bss)
	}
	USER_END = .;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }	
}
