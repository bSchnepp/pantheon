#include <kern.h>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "gic.hpp"

static UINT8 NumSockets = 0;
static UINT64 NumInterrupts = 0;
static BOOL SecurityExtensions = FALSE;

static UINT64 GICAddresses[pantheon::arm::GIC_CLASS_TYPE_MAX];

VOID pantheon::arm::GICSetMMIOAddr(pantheon::arm::GICClassType Type, UINT64 Addr)
{
	GICAddresses[Type] = Addr;
}

/* 
 * The GIC can be thought up as having a FSM attached for up to 1024 interrupt
 * lines. That DFA, as pseudocode, can be visualized as:
 * 
 * INACTIVE : SIG -> PENDING
 * PENDING : RECV -> ACTIVE
 * ACTIVE : ACK -> INACTIVE
 * ACTIVE : SIG -> ACTIVE_PENDING
 * ACTIVE_PENDING : ACK -> PENDING
 */

VOID pantheon::arm::GICInit()
{
	/* TODO: Create functions to more easilly get this info.*/
	UINT32 TypeR = GICRead(GIC_CLASS_DISTRIBUTOR, GICD_TYPER, 0);
	NumInterrupts = 32 * ((TypeR & 0x1F) + 1);
	NumSockets = ((TypeR >> 5) & 0b111) + 1;
	SecurityExtensions = ((TypeR >> 10) & 0x01);

	/* Enable group0 and group1 */
	UINT32 CTLR = (1 << 0) | (1 << 1);
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, CTLR);

	for (UINT32 Index = 0; Index < (NumInterrupts / 32); ++Index)
	{
		/* Disable it first, just in case. */
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ICENABLER, Index, 0xFFFFFFFF);

		/* While here, also ensure it's going to group 0 always. */
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IGROUPR, Index, 0x00000000);
	}

	/* Everything goes to interface 0, with the same priority. */
	for (UINT64 Index = 0; Index < NumInterrupts / 4; ++Index)
	{
		if (Index >= 8)
		{
			GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ITARGETSR, Index, 0x01010101);
		}
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Index, 0x00000000);
	}
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_ISENABLER, 0, 0xFFFFFFFF);
}

UINT8 pantheon::arm::GICGetNumSockets()
{
	return NumSockets;
}

VOID pantheon::arm::GICInitCore()
{
	for (UINT8 Index = 0; Index < 8; ++Index)
	{
		/* Everything's priority needs to be 0. */
		GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IPRIORITYR, Index, 0x00000000);
	}

	/* Everything's group is 0. */
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_IGROUPR, 0, 0x00000000);

	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0xFF);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x07);

	GICEnable();
}

VOID pantheon::arm::GICEnable()
{
	/* Refer to GIC spec. Enable group 0, 1, and AckCtrl */
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, 0x05);
}

VOID pantheon::arm::GICDisable()
{
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, 0);
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

VOID pantheon::arm::GICIgnoreInterrupt(UINT32 Interrupt)
{
	UINT32 Interface = Interrupt / 32;
	UINT32 IRQNumber = Interrupt % 32;

	GICWrite(GIC_CLASS_DISTRIBUTOR, 
		GICD_ICPENDR, Interface, (1 << IRQNumber));	
}

VOID pantheon::arm::GICAckInterrupt(UINT32 Value)
{
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_EOIR, 0, Value);
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

VOID pantheon::arm::GICSetInterface(UINT32 Interrupt, UINT32 Value)
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

UINT32 pantheon::arm::GICRecvInterrupt()
{
	return GICRead(GIC_CLASS_CPU_INTERFACE, GICC_IAR, 0);
}