ENTRY(kern_init)

SECTIONS
{
	/* aarch64 load address */
	VIRT_LOAD_ADDRESS = 0xFFFFFFFF70000000;
	. += VIRT_LOAD_ADDRESS;

	. = ALIGN(0x1000);
	. = ALIGN(0x800);
	TEXT_AREA = .;
	TEXT_PHY_AREA = . - VIRT_LOAD_ADDRESS;
	kern_begin = .;	
	.text : AT(ADDR(.text) - VIRT_LOAD_ADDRESS)
	{
		*(.text.exceptions)
		. = ALIGN(0x1000);
		*(.text)
	}
	. = ALIGN(0x1000);
	TEXT_END = .;

	. = ALIGN(0x1000);
	RODATA_AREA = .;
	RODATA_PHY_AREA = . - VIRT_LOAD_ADDRESS;
	.rodata : AT(ADDR(.rodata) - VIRT_LOAD_ADDRESS)
	{
		*(.rodata*)
	}
	. = ALIGN(0x1000);
	RODATA_END = .;

	. = ALIGN(0x1000);
	DATA_AREA = .;
	DATA_PHY_AREA = . - VIRT_LOAD_ADDRESS;
	.data : AT(ADDR(.data) - VIRT_LOAD_ADDRESS)
	{
		*(.data*)
		. = ALIGN(0x1000);
	}
	DATA_END = .;

	BSS_AREA = .;
	BSS_PHY_AREA = . - VIRT_LOAD_ADDRESS;
	.bss : AT(ADDR(.bss) - VIRT_LOAD_ADDRESS)
	{
		. = ALIGN(0x1000);
		*(.bss.early)
		*(.bss*)

		/* Hack for now to make this work for us. */
		kern_stack_begin = .;
		. += 32 * 1024;
		kern_stack_end = .;
		kern_stack_size = kern_stack_end - kern_stack_begin;
		. += 0x1000;	/* Add extra space just in case... */
		. = ALIGN(4 * 1024);
		kern_end = .;	
	}
	BSS_END = .;
	BSS_SIZE = BSS_END - BSS_AREA;

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }	
}
