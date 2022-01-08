#ifndef _ARCH_HPP_
#define _ARCH_HPP_

#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>
#include <arch/aarch64/vmm/vmm.hpp>

#include <arch/aarch64/sync.hpp>

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

FORCE_INLINE UINT64 R_TTBR0_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, ttbr0_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}

FORCE_INLINE UINT64 R_TTBR1_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, ttbr1_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}

FORCE_INLINE VOID W_TTBR0_EL1(UINT64 Val)
{
	asm volatile ("msr ttbr0_el1, %0\n" :: "r"(Val) :);
}

FORCE_INLINE VOID W_TTBR1_EL1(UINT64 Val)
{
	asm volatile ("msr ttbr1_el1, %0\n" :: "r"(Val) :);
}

FORCE_INLINE VOID W_MAIR_EL1(UINT64 Value)
{
	asm volatile ("msr mair_el1, %0\n" :: "r"(Value) :);
}

FORCE_INLINE UINT64 R_MAIR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mair_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}

FORCE_INLINE VOID W_TCR_EL1(UINT64 Value)
{
	asm volatile ("msr tcr_el1, %0\n" :: "r"(Value) :);
}

FORCE_INLINE UINT64 R_TCR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, tcr_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}


}



}

extern "C" void enable_interrupts();

#endif