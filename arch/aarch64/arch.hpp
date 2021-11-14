#ifndef _ARCH_HPP_
#define _ARCH_HPP_

#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>
#include <arch/aarch64/vmm/vmm.hpp>

typedef struct PagingInfo
{
	UINT64 TTBR0;
	UINT64 TTBR1;
}PagingInfo;

namespace pantheon
{

typedef pantheon::arm::CpuContext CpuContext;
typedef pantheon::arm::TrapFrame TrapFrame;

FORCE_INLINE VOID RearmSystemTimer(UINT64 Freq)
{
	pantheon::arm::RearmSystemTimer(Freq);
}

FORCE_INLINE VOID DisableSystemTimer()
{
	pantheon::arm::DisableSystemTimer();
}

FORCE_INLINE VOID RearmSystemTimer()
{
	pantheon::arm::RearmSystemTimer();
}

namespace CPU
{

/**
 * \~english @brief Gets the processor number of the current core
 * \~english @author Brian Schnepp
 */
UINT8 GetProcessorNumber();

VOID CLI();
VOID STI();
VOID HLT();
BOOL IF();

VOID PUSHI();
VOID POPI();
UINT64 ICOUNT();

VOID PAUSE();
VOID LIDT(void *IDT);

}

namespace CPUReg
{

UINT64 R_TTBR0_EL1();
UINT64 R_TTBR1_EL1();

VOID W_TTBR0_EL1(UINT64);
VOID W_TTBR1_EL1(UINT64);

VOID W_MAIR_EL1(UINT64 Value);
UINT64 R_MAIR_EL1();

VOID W_TCR_EL1(UINT64 Value);
UINT64 R_TCR_EL1();

}

namespace Sync
{
	FORCE_INLINE void DSBISH()
	{
		asm volatile("dsb ish\n" ::: "memory");
	}

	FORCE_INLINE void DSBSY()
	{
		asm volatile("dsb sy\n" ::: "memory");
	}

	FORCE_INLINE void ISB()
	{
		asm volatile("isb\n" ::: "memory");
	}
}

}

extern "C" void enable_interrupts();

#endif