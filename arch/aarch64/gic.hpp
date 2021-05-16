#include <kern_datatypes.hpp>

#ifndef _PANTHEON_ARM_GIC_HPP_
#define _PANTHEON_ARM_GIC_HPP_

namespace pantheon::arm
{

typedef enum GICClassType
{
	GIC_CLASS_DISTRIBUTOR = 0x1000,
	GIC_CLASS_CPU_INTERFACE = 0x2000,
	GIC_CLASS_VIRTUAL_CONTROL = 0x4000,
	GIC_CLASS_VIRTUAL_CPU = 0x5000,
	GIC_CLASS_VIRTUAL_CPU_INTERFACES = 0x6000,
}GICClassType;

typedef enum GICRegisterOffsets
{
	/*  Distributor register map */
	GICD_CTLR = 0x00,
	GICD_TYPER = 0x04,
	GICD_IIDR = 0x08,

	/* Array of 32 registers each */
	GICD_IGROUPR = 0x080,
	GICD_ISENABLER = 0x100,
	GICD_ICENABLER = 0x180,
	GICD_ISPENDR = 0x200,
	GICD_ICPENDR = 0x280,
	GICD_ISACTIVER = 0x300,
	GICD_ICACTIVER = 0x380,
	GICD_IPRIORITYR = 0x400,

	GICD_ITARGETSR = 0x800,
	GICD_ICFGR = 0xC00,

	GICD_NSCAR = 0xE00,
	GICD_SGIR = 0xF00,

	GICD_CPENDSGIR = 0xF10,
	GICD_SPENDSGIR = 0xF20,

	/* CPU interface register map */
	GICC_CTLR = 0x00,
	GICC_PMR = 0x04,
	GICC_BPR = 0x08,
	GICC_IAR = 0x0C,
	GICC_EOIR = 0x10,
	GICC_RPR = 0x14,
	GICC_HPPIR = 0x18,
	GICC_ABPR = 0x1C,
	GICC_AIAR = 0x20,
	GICC_AEOIR = 0x24,
	GICC_AHPPIR = 0x28,
	GICC_APR = 0xD0,
	GICC_NSAPR = 0xE0,
	GICC_IIDR = 0xFC,
	GICC_DIR = 0x1000,

	/* Hypervisor stuff */
	GICH_HCR = 0x00,
	GICH_VTR = 0x04,
	GICH_VMCR = 0x08,
	GICH_MISR = 0x10,
	GICH_EISR0 = 0x20,
	GICH_EISR1 = 0x24,
	GICH_ELSR0 = 0x30,
	GICH_ELSR1 = 0x34,
	GICH_APR = 0xF0,
	GICH_LR = 0x100,

	GICV_CTLR = 0x00,
	GICV_PMR = 0x04,
	GICV_BPR = 0x08,
	GICV_IAR = 0x0C,
	GICV_EOIR = 0x10,
	GICV_RPR = 0x14,
	GICV_HPPIR = 0x18,
	GICV_ABPR = 0x1C,
	GICV_AIAR = 0x20,
	GICV_AEOIR = 0x24,
	GICV_AHPPIR = 0x28,
	GICV_APR = 0xD0,
	GICV_IIDR = 0xFC,
	GICV_DIR = 0x1000,
}GICRegisterOffsets;

typedef struct GICv2_GICD_IIDR
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT8 ProductID;
			UINT8 Reserved : 4;
			UINT8 Variant : 4;
			UINT8 Revision : 4;
			UINT16 Implementer : 12;
		};
	};
}__attribute__((__packed__)) GICv2_GICD_IIDR;

typedef struct GICv2_GICD_PPISR
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Reserved1;
			BOOL ID31Status : 1;
			BOOL ID30Status : 1;
			BOOL ID29Status : 1;
			BOOL ID28Status : 1;
			BOOL ID27Status : 1;
			BOOL ID26Status : 1;
			BOOL ID25Status : 1;
			UINT8 Reserved0;
		};
	};
}__attribute__((__packed__)) GICv2_GICD_PPISR;

typedef struct GICv2_GICD_ICFGRn
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Reserved1;
			UINT8 PPIStatus;
			UINT8 Reserved0;
		};
	};
}__attribute__((__packed__)) GICv2_GICD_ICFGRn;

typedef struct GICv2_GICD_SPISRn
{
	union
	{
		UINT32 Raw;
		struct
		{
			BOOL ThirtyOne : 1;
			BOOL Thirty : 1;
			BOOL TwentyNine : 1;
			BOOL TwentyEight : 1;
			BOOL TwentySeven : 1;
			BOOL TwentySix : 1;
			BOOL TwentyFive : 1;
			BOOL TwentyFour : 1;
			BOOL TwentyThree : 1;
			BOOL TwentyTwo : 1;
			BOOL TwentyOne : 1;
			BOOL Twenty : 1;			
			BOOL Nineteen : 1;
			BOOL Eighteen : 1;
			BOOL Seventeen : 1;
			BOOL Sixteen : 1;
			BOOL Fifteen : 1;
			BOOL Fourteen : 1;
			BOOL Thirteen : 1;
			BOOL Twelve : 1;
			BOOL Eleven : 1;
			BOOL Ten : 1;
			BOOL Nine : 1;
			BOOL Eight : 1;
			BOOL Seven : 1;
			BOOL Six : 1;
			BOOL Five : 1;
			BOOL Four : 1;
			BOOL Three : 1;
			BOOL Two : 1;
			BOOL One : 1;
			BOOL Zero : 1;
		};
	};
}__attribute__((__packed__)) GICv2_GICD_SPISRn;

typedef struct GICv2_GICH_VTR
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT8 PRIBits : 3;
			UINT8 PREBits : 3;
			UINT32 Reserved : 20;
			UINT8 ListRegs : 6;
		};
	};
}__attribute__((__packed__)) GICv2_GICH_VTR;

VOID GICSetBaseAddr(UINT64 Address);

VOID GICInit();
VOID GICInitCore();

VOID GICEnable();
VOID GICDisable();

UINT32 GICRead(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex);

VOID GICWrite(GICClassType Type, 
	GICRegisterOffsets Offset, UINT32 ArrayIndex, UINT32 Value);

VOID GICEnableInterrupt(UINT32 Interrupt);
VOID GICDisableInterrupt(UINT32 Interrupt);
VOID GICAckInterrupt(UINT32 Interrupt);

UINT8 GICGetNumCPUs();



}


#endif