ENTRY(_cpu_start)

SECTIONS
{
	/* On qemu-aarch64, this is the load address with -kernel */
	BSP_LOAD_ADDRESS = 0x80000 + 0x40000000;
	. = BSP_LOAD_ADDRESS;

	PRE_KERN_AREA = .;
	. = ALIGN(0x1000);
	.text.early :
	{
		*(.text.early)
		BoardSupport/qemu-aarch64/libBoard.a*(*.text*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.text*)
	}

	. = ALIGN(0x1000);
	.rodata.early :
	{
		*(.rodata.early)
		BoardSupport/qemu-aarch64/libBoard.a*(*.rodata*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.rodata*)
	}

	.data.early :
	{
		. = ALIGN(0x1000);
		*(.data.early)
		BoardSupport/qemu-aarch64/libBoard.a*(*.data*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.data*)

		. = ALIGN(0x1000);
		*(.bss.early)
		BoardSupport/qemu-aarch64/libBoard.a*(*.bss*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.bss*)
	}
	PRE_KERN_END = .;

	. = ALIGN(4 * 1024);
	USER_BEGIN = .;
	.user.text : 
	{
		Userland/libUserland.a*(*.text*)
	}

	.user.rodata : 
	{
		Userland/libUserland.a*(*.rodata*)
	}

	.user.data : 
	{
		Userland/libUserland.a*(*.data*)
		Userland/libUserland.a*(*.bss*)
	}
	USER_END = .;

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
	.data : AT(ADDR(.data) - VIRT_LOAD_ADDRESS)
	{
		DATA_AREA = .;
		DATA_PHY_AREA = . - VIRT_LOAD_ADDRESS;
		*(.data*)
		. = ALIGN(0x1000);
		DATA_END = .;

		. = ALIGN(0x1000);
		BSS_AREA = .;
		BSS_PHY_AREA = . - VIRT_LOAD_ADDRESS;
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
		BSS_END = .;
		BSS_SIZE = BSS_END - BSS_AREA;
	}

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }	
}
