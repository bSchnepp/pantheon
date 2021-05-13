#include <kern.h>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "gic.hpp"

static UINT64 MMIOAddr;

static UINT8 NumCPUs = 0;
static UINT32 InterruptLines = 0;
static BOOL SecurityExtn = FALSE;

VOID pantheon::arm::GICSetBaseAddr(UINT64 Address)
{
	MMIOAddr = Address;
}

VOID pantheon::arm::GICInit()
{
	WriteMMIOS32(MMIOAddr + GIC_CLASS_DISTRIBUTOR + GICD_CTLR, 1);
	WriteMMIOS32(MMIOAddr + GIC_CLASS_CPU_INTERFACE + GICC_CTLR, 1);

	NumCPUs = ((GICRead(GIC_CLASS_DISTRIBUTOR, GICD_TYPER, 0) >> 5) & 0x0F) + 1;
	InterruptLines = 32 * ((GICRead(GIC_CLASS_DISTRIBUTOR, GICD_TYPER, 0) & 0x1F) + 1);
	SecurityExtn = (GICRead(GIC_CLASS_DISTRIBUTOR, GICD_TYPER, 0) & 0x1) >> 10;
	SERIAL_LOG("got %hhu cpus and %d ints\n", NumCPUs, InterruptLines);
	SERIAL_LOG("security extensions: %hhu\n", SecurityExtn);

	if (SecurityExtn)
	{
		/* enable both groups i guesss */
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, 0x03);
	} else {
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, 0x01);
	}

	/* all interrupts are diabled for now, then re-enabled in a bit. */
	/* And set them all up for being on group 0, which is always present. */
	for (UINT32 Index = 0; Index < InterruptLines / 32; ++Index)
	{
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ICENABLER, Index, 0xFFFFFFFF);
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IGROUPR, Index, 0x00000000);
	}

	/* Everything must go to CPU 0, with the same priority */
	for (UINT32 Index = 0; Index < InterruptLines / 4; ++Index)
	{
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Index, 0x01010101);
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Index, 0x00000000);
	}

	/* TODO: Call on other cores. */
	GICInitCore();
}

UINT8 pantheon::arm::GICGetNumCPUs()
{
	return NumCPUs;
}

VOID pantheon::arm::GICInitCore()
{
	/* Enable banked interrupts */
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ISENABLER, 0, 0xFFFFFFFF);
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IGROUPR, 0, 0x00000000);

	/* iprority0 - 7 are banked for each processor */
	for (UINT32 Index = 0; Index < 8; ++Index)
	{
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Index, 0x00000000);
	}

	SERIAL_LOG("%s\n", "core was inited");
}

VOID pantheon::arm::GICEnable()
{
	WriteMMIOS32(MMIOAddr + GIC_CLASS_CPU_INTERFACE + GICC_CTLR, 1);
	WriteMMIOS32(MMIOAddr + GIC_CLASS_DISTRIBUTOR + GICD_CTLR, 1);
	asm volatile ("msr daifclr, #15\n");

	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0xFF);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x00);
	SERIAL_LOG("%s\n", "gic enabled");
}

VOID pantheon::arm::GICDisable()
{
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0x00);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x00);

	asm volatile ("msr daifset, #15\n");
	WriteMMIOS32(MMIOAddr + GIC_CLASS_CPU_INTERFACE + GICC_CTLR, 0);
	WriteMMIOS32(MMIOAddr + GIC_CLASS_DISTRIBUTOR + GICD_CTLR, 0);
}

UINT32 pantheon::arm::GICRead(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex)
{
	UINT64 Value = MMIOAddr + Offset + Type + (ArrayIndex * 4);
	return ReadMMIOU32(Value);
}

VOID pantheon::arm::GICWrite(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex, UINT32 Value)
{
	UINT64 Addr = MMIOAddr + Offset + Type + (ArrayIndex * 4);
	WriteMMIOU32(Addr, Value);
}

VOID pantheon::arm::GICLoadInterruptTable(VOID *Table)
{
	asm volatile("msr vbar_el1, %x[Addr]\n"
		: [Addr]"+r"(Table));
}

VOID pantheon::arm::GICEnableInterrupt(UINT32 Interrupt)
{
	UINT32 Interface = Interrupt / 32;
	UINT32 IRQNumber = Interrupt % 32;

	GICWrite(GIC_CLASS_DISTRIBUTOR, 
		GICD_ISENABLER, Interface, (1 << IRQNumber));
}

VOID pantheon::arm::GICDisableInterrupt(UINT32 Interrupt)
{
	UINT32 Interface = Interrupt / 32;
	UINT32 IRQNumber = Interrupt % 32;

	GICWrite(GIC_CLASS_DISTRIBUTOR, 
		GICD_ICENABLER, Interface, (1 << IRQNumber));
}

VOID pantheon::arm::GICAckInterrupt(UINT32 Interrupt)
{
	UINT32 Interface = Interrupt / 32;
	UINT32 IRQNumber = Interrupt % 32;

	GICWrite(GIC_CLASS_DISTRIBUTOR, 
		GICD_ICPENDR, Interface, (1 << IRQNumber));
}