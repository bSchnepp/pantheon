ENTRY(_start)

SECTIONS
{
	.text : ALIGN(4K)
	{
		*(.text*)
	}

	.rodata : ALIGN(4K)
	{
		*(.rodata*)
	}

	.data : ALIGN(4K)
	{
		*(.data*)
	}

	.bss : ALIGN(4K)
	{
		*(.bss*)
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

	.dynamic : { *(.dynamic) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.rela.dyn : { *(.rela.dyn) }

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }	
}
