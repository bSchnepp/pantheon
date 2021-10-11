#include <kern_macro.hpp>
#include <kern_datatypes.hpp>

#ifndef _AARCH64_MMU_HPP_
#define _AARCH64_MMU_HPP_

namespace pantheon::arm
{

typedef enum MAIRAttribute : UINT64
{
	MAIR_ATTRIBUTE_FEAT_NC = 0b01,

	MAIR_ATTRIBUTE_NORMAL_INNER_NONCACHEABLE = (0b1ULL << 6),
	MAIR_ATTRIBUTE_NORMAL_OUTER_NONCACHEABLE = (0b1ULL << 2),

	MAIR_ATTRIBUTE_DEVICE_NGNRNE = 0b00ULL << 2,
	MAIR_ATTRIBUTE_DEVICE_NGNRE = 0b01ULL << 2,
	MAIR_ATTRIBUTE_DEVICE_NGRE = 0b10ULL << 2,
	MAIR_ATTRIBUTE_DEVICE_GRE = 0b11ULL << 2,
}MAIRAttribute;

typedef UINT64 MAIRAttributes;

constexpr inline MAIRAttributes AttributeToSlot(MAIRAttributes Attribs, UINT8 Slot)
{
	return Attribs << (Slot * 8);
}

typedef enum TCRAttribute : UINT64
{
	/* We only support 2^48 virtual addressing. No 5 level page tables here! */
	TCR_ATTRIBUTE_T0SZ_4LVL = (64 - 48) << 0,
	TCR_ATTRIBUTE_T1SZ_4LVL = (64 - 48) << 16,

	/* We probably only want to use 4K, but there's others just in case. */
	TCR_ATTRIBUTE_TG0_4K = (0b00ULL) << 14,
	TCR_ATTRIBUTE_TG0_16K = (0b10ULL) << 14,
	TCR_ATTRIBUTE_TG0_64K = (0b01ULL) << 14,

	/* Likewise for TG1 */
	TCR_ATTRIBUTE_TG1_4K = (0b10ULL) << 30,
	TCR_ATTRIBUTE_TG1_16K = (0b01ULL) << 30,
	TCR_ATTRIBUTE_TG1_64K = (0b11ULL) << 30,
}TCRAttribute;

typedef UINT64 TCRAttributes;

constexpr inline TCRAttributes DefaultTCRAttributes()
{
	return TCR_ATTRIBUTE_T0SZ_4LVL | TCR_ATTRIBUTE_TG0_4K | TCR_ATTRIBUTE_T1SZ_4LVL | TCR_ATTRIBUTE_TG1_4K;
}

VOID WriteMAIR_EL1(UINT64 Value);
UINT64 ReadMAIR_EL1();

VOID WriteTCR_EL1(UINT64 Value);
UINT64 ReadTCR_EL1();


}


#endif