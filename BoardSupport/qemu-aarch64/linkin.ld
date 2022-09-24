ENTRY(_cpu_start)

SECTIONS
{
	/* On qemu-aarch64, this is the load address with -kernel */
	BSP_LOAD_ADDRESS = 0x80000 + 0x40000000;
	. = BSP_LOAD_ADDRESS;

	PRE_KERN_AREA = .;
	. = ALIGN(0x1000);
	.text :
	{
		TEXT_PHY_AREA = .;
		TEXT_AREA = .;
		*(.text*)
		BoardSupport/qemu-aarch64/libBoard.a*(*.text*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.text*)
		TEXT_END = .;
		TEXT_SIZE = TEXT_END - TEXT_AREA;
	}

	. = ALIGN(0x1000);
	.rodata :
	{
		RODATA_PHY_AREA = .;
		RODATA_AREA = .;
		. = ALIGN(0x1000);
		*(.rodata*)
		BoardSupport/qemu-aarch64/libBoard.a*(*.rodata*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.rodata*)
		RODATA_END = .;
		RODATA_SIZE = RODATA_END - RODATA_AREA;
	}

	.data :
	{
		DATA_PHY_AREA = .;
		DATA_AREA = .;
		. = ALIGN(0x1000);
		*(.data*)
		BoardSupport/qemu-aarch64/libBoard.a*(*.data*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.data*)
		DATA_END = .;
		DATA_SIZE = DATA_END - DATA_AREA;

		BSS_PHY_AREA = .;
		BSS_AREA = .;
		. = ALIGN(0x1000);
		*(.bss*)
		BoardSupport/qemu-aarch64/libBoard.a*(*.bss*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.bss*)
		BSS_END = .;
		BSS_SIZE = BSS_END - BSS_AREA;
	}
	PRE_KERN_END = .;	
}
