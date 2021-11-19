#include "arch.hpp"

VOID pantheon::CPUReg::W_MAIR_EL1(UINT64 Value)
{
	asm volatile ("msr mair_el1, %0\n" :: "r"(Value) :);
}

UINT64 pantheon::CPUReg::R_MAIR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mair_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}

VOID pantheon::CPUReg::W_TCR_EL1(UINT64 Value)
{
	asm volatile ("msr tcr_el1, %0\n" :: "r"(Value) :);
}

UINT64 pantheon::CPUReg::R_TCR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, tcr_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}
