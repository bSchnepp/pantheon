ENTRY(kern_init)

PHDRS
{
	dyn PT_DYNAMIC;
}

SECTIONS
{
	/* aarch64 load address */

	kern_begin = .;	
	.text : ALIGN(4K)
	{
		TEXT_AREA = .;
		METADATA_BEGIN = .;
		KEEP(*(.text.metadata))
		METADATA_END = .;
		
		. = ALIGN(0x800);
		KEEP(*(.text.exceptions))
		. = ALIGN(0x1000);
		*(.text)
		*(.text.*)
		TEXT_END = .;
		TEXT_SIZE = TEXT_END - TEXT_AREA;
	}

	.rodata : ALIGN(4K)
	{
		RODATA_AREA = .;
		KEEP(*(.rodata*))
	}

	.preinit_array : ALIGN(4K)
	{
		PREINIT_ARRAY_AREA = .;
		KEEP(*(.preinit_array))
		PREINIT_ARRAY_END = .;
	}	

	.init_array : ALIGN(4K)
	{
		INIT_ARRAY_AREA = .;
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		INIT_ARRAY_END = .;
		RODATA_END = .;
	}

	.data : ALIGN(4K)
	{
		DATA_AREA = .;
		KEEP(*(.data*))
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

	.dynamic : ALIGN(4K) { *(.dynamic) } :dyn
	.dynsym : ALIGN(4K) { *(.dynsym) }
	.dynstr : ALIGN(4K) { *(.dynstr) }
	.rela.dyn : ALIGN(4K) { *(.rela.*) }

	/* Associate these other areas under DATA to make sure it's not 
	 * overwritten */
	DATA_END = .;
	DATA_SIZE = TEXT_END - DATA_AREA;
	. = ALIGN(4K);

	.bss : ALIGN(4K)
	{
		BSS_AREA = .;
		KEEP(*(.bss.early))
		KEEP(*(.bss*))

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
