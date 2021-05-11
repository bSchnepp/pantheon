#include <kern_datatypes.hpp>
#include "kern_cpu.hpp"

UINT8 pantheon::CPU::GetProcessorNumber()
{
	UINT64 RetVal;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(RetVal) ::);
	return (UINT8)(RetVal & 0xFF);
}