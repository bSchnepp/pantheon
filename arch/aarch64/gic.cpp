#include <kern.h>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "gic.hpp"

static UINT8 NumCPUs = 0;
static UINT64 NumInterrupts = 0;
static UINT64 GICAddresses[pantheon::arm::GIC_CLASS_TYPE_MAX];

VOID pantheon::arm::GICSetMMIOAddr(pantheon::arm::GICClassType Type, UINT64 Addr)
{
	GICAddresses[Type] = Addr;
}

VOID pantheon::arm::GICInit()
{
	GICv2_GICD_TYPER TypeR;

	TypeR.Raw = GICRead(GIC_CLASS_DISTRIBUTOR, GICD_TYPER, 0);
	NumInterrupts = 32 * (TypeR.ITLinesNumber + 1);
	NumCPUs = TypeR.CpuNumber + 1;

	/* Don't permit interrupts from firing when we're setting up.*/
	pantheon::arm::GICDisable();

	/* Forcefully ensure everything's enabled. */
	for (UINT8 Index = 0; Index < TypeR.ITLinesNumber; ++Index)
	{
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ISENABLER, Index, 0xFFFFFFFF);
	}
}

UINT8 pantheon::arm::GICGetNumCPUs()
{
	return NumCPUs;
}

VOID pantheon::arm::GICInitCore()
{
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ISENABLER, 0, 0xFFFFFFFF);

	for (UINT8 Index = 0; Index < 8; ++Index)
	{
		/* Everything's priority needs to be 0. */
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Index * 4, 0x00000000);
	}

	/* Everything's group is 0. */
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IGROUPR, 0, 0x00000000);

	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0xFF);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x07);

	GICEnable();
}

VOID pantheon::arm::GICEnable()
{
	UINT32 GICCControl = 0;
	GICCControl |= (0 << 9); /* Set EOImodeNS */
	GICCControl |= (1 << 9); /* Set common binary point */
	GICCControl |= (0 << 4); /* Set CBPR enabled */
	GICCControl |= (0 << 3); /* Set FIQ enabled */
	GICCControl |= (1 << 2); /* Set ACK control */
	GICCControl |= (1 << 1); /* Enable group 1 */
	GICCControl |= (1 << 0); /* Enable group 0 */
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, GICCControl);
	
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, 0x03);
}

VOID pantheon::arm::GICDisable()
{
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, FALSE);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, FALSE);
}

UINT32 pantheon::arm::GICRead(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex)
{
	UINT64 Value = GICAddresses[Type] + Offset + (ArrayIndex * 4);
	return ReadMMIOU32(Value);
}

VOID pantheon::arm::GICWrite(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex, UINT32 Value)
{
	UINT64 Addr = GICAddresses[Type] + Offset + (ArrayIndex * 4);
	WriteMMIOU32(Addr, Value);
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

BOOL pantheon::arm::GICPollInterrupt(UINT32 Interrupt)
{
	UINT32 Interface = Interrupt / 32;
	UINT32 IRQNumber = Interrupt % 32;

	UINT32 Res = GICRead(GIC_CLASS_DISTRIBUTOR, 
		GICD_ICPENDR, Interface);
	return Res & (1 << IRQNumber);
}

VOID pantheon::arm::GICSetConfig(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 2 * (Interrupt % 16);
	UINT32 NANDValue = 0x03 << Shift;

	UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_ICFGR, Interrupt / 16);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ICFGR, Interrupt / 16, CurValue);

}

VOID pantheon::arm::GICSetPriority(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 8 * (Interrupt % 4);
	UINT32 NANDValue = 0xFF << Shift;

	UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Interrupt / 4);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Interrupt / 16, CurValue);
}

VOID pantheon::arm::GICSetCore(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 8 * (Interrupt % 4);
	UINT32 NANDValue = 0xFF << Shift;

	UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Interrupt / 4);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Interrupt / 16, CurValue);
}

UINT64 pantheon::arm::GICGetNumInterrupts()
{
	return NumInterrupts;
}