#include "mmu.hpp"

VOID pantheon::arm::WriteMAIR_EL1(UINT64 Value)
{
	asm volatile ("msr mair_el1, %0\n" :: "r"(Value) :);
}

UINT64 pantheon::arm::ReadMAIR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mair_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}

VOID pantheon::arm::WriteTCR_EL1(UINT64 Value)
{
	asm volatile ("msr tcr_el1, %0\n" :: "r"(Value) :);
}

UINT64 pantheon::arm::ReadTCR_EL1()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, tcr_el1\n" : "=r"(RetVal) ::);
	return RetVal;
}
