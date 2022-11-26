ENTRY(_cpu_start)

PHDRS
{
	dyn PT_DYNAMIC;
}

SECTIONS
{
	PRE_KERN_AREA = .;
	. = ALIGN(0x1000);
	.text :
	{
		KEEP(*(.text.early))
		KEEP(*(.text))
		. = ALIGN(0x1000);
		BoardSupport/qemu-aarch64/libBoard.a*(*.text*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.text*)
		. = ALIGN(0x1000);
	}

	. = ALIGN(0x1000);
	.rodata :
	{
		KEEP(*(.rodata*))
		. = ALIGN(0x1000);
		BoardSupport/qemu-aarch64/libBoard.a*(*.rodata*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.rodata*)
		. = ALIGN(0x1000);
	}

	. = ALIGN(0x1000);
	.data :
	{
		. = ALIGN(0x1000);
		KEEP(*(.data.kernel*))
		. = ALIGN(0x1000);
		KEEP(*(.data*))
		. = ALIGN(0x1000);
		BoardSupport/qemu-aarch64/libBoard.a*(*.data*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.data*)
		. = ALIGN(0x1000);
	}

	. = ALIGN(0x1000);
	.bss :
	{
		BSS_AREA = .;
		KEEP(*(.bss*))
		. = ALIGN(0x1000);
		BoardSupport/qemu-aarch64/libBoard.a*(*.bss*)
		BoardSupport/qemu-aarch64/Boot/libBoot.a*(*.bss*)
		. = ALIGN(0x1000);
		BSS_END = .;
	}

	.plt : ALIGN(4K)
	{
		*(.plt)
		*(.iplt)
	}

	.got : ALIGN(4K)
	{
		*(.got)
		*(.igot)
	}

	.got.plt : ALIGN(4K)
	{
		*(.got.plt)
		*(.igot.plt)
	}

	.dynamic : { KEEP(*(.dynamic)) } :dyn
	.dynsym : { KEEP(*(.dynsym)) }
	.dynstr : { KEEP(*(.dynstr)) }
	.rela.dyn : { KEEP(*(.rela.dyn)) }

	PRE_KERN_END = .;	
}
