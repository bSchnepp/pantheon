#include <kern.h>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "gic.hpp"

static UINT8 NumCPUs = 0;
static UINT64 GICAddresses[pantheon::arm::GIC_CLASS_TYPE_MAX];

VOID pantheon::arm::GICSetMMIOAddr(pantheon::arm::GICClassType Type, UINT64 Addr)
{
	GICAddresses[Type] = Addr;
}

VOID pantheon::arm::GICInit()
{
	pantheon::arm::GICEnable();
}

UINT8 pantheon::arm::GICGetNumCPUs()
{
	return NumCPUs;
}

VOID pantheon::arm::GICInitCore()
{
}

VOID pantheon::arm::GICEnable()
{
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, TRUE);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, TRUE);

	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0xFF);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x00);
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

VOID pantheon::arm::GICSetConfig(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 2 * (Interrupt % 16);
	UINT32 NANDValue = 0x03 << Shift;

	volatile UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_ICFGR, Interrupt / 16);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ICFGR, Interrupt / 16, CurValue);

}

VOID pantheon::arm::GICSetPriority(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 8 * (Interrupt % 4);
	UINT32 NANDValue = 0xFF << Shift;

	volatile UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Interrupt / 4);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Interrupt / 16, CurValue);
}

VOID pantheon::arm::GICSetCore(UINT32 Interrupt, UINT32 Value)
{
	UINT32 Shift = 8 * (Interrupt % 4);
	UINT32 NANDValue = 0xFF << Shift;

	volatile UINT32 CurValue = pantheon::arm::GICRead(
		GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Interrupt / 4);

	CurValue &= ~NANDValue;
	CurValue |= (Value << Shift);
	pantheon::arm::GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Interrupt / 16, CurValue);
}