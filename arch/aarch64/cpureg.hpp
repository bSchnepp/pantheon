#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _ARCH_CPU_REG_HPP_
#define _ARCH_CPU_REG_HPP_


namespace pantheon::CPUReg
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
	/* The TLB has to also be smashed here... */
	asm volatile ("msr ttbr0_el1, %0\n"
		"tlbi vmalle1is\n"
		"dsb ish\n"
		"isb\n" :: "r"(Val) :);
}

FORCE_INLINE VOID W_TTBR1_EL1(UINT64 Val)
{
	/* The TLB has to also be smashed here... */
	asm volatile ("msr ttbr1_el1, %0\n"
		"tlbi vmalle1is\n"
		"dsb ish\n"
		"isb\n"  :: "r"(Val) :);
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

FORCE_INLINE VOID W_TPIDRRO_EL0(UINT64 Value)
{
	asm volatile ("msr tpidrro_el0, %0\n" :: "r"(Value) :);
}

FORCE_INLINE UINT64 R_TPIDRRO_EL0()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, tpidrro_el0\n" : "=r"(RetVal) ::);
	return RetVal;
}


}

#endif