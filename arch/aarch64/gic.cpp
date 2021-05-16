#include <kern.h>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "gic.hpp"


static UINT64 GICAddresses[pantheon::arm::GIC_CLASS_TYPE_MAX];


static UINT8 NumCPUs = 0;
static UINT32 InterruptLines = 0;
static BOOL SecurityExtn = FALSE;

VOID pantheon::arm::GICSetMMIOAddr(pantheon::arm::GICClassType Type, UINT64 Addr)
{
	GICAddresses[Type] = Addr;
}

VOID pantheon::arm::GICInit()
{
	
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
	GICWrite(GIC_CLASS_DISTRIBUTOR, GICD_CTLR, 0, TRUE);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_CTLR, 0, TRUE);

	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_PMR, 0, 0xFF);
	GICWrite(GIC_CLASS_CPU_INTERFACE, GICC_BPR, 0, 0x00);

	GICInitCore();
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