#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#include <Proc/kern_cpu.hpp>
#include <Common/Sync/kern_atomic.hpp>

UINT8 pantheon::CPU::GetProcessorNumber()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(RetVal) ::);
	return (UINT8)(RetVal & 0xFF);
}

VOID pantheon::CPU::CLI()
{
	asm volatile("msr daifset, #2\n");
}

VOID pantheon::CPU::STI()
{
	asm volatile("msr daifclr, #2\n");
}

VOID pantheon::CPU::PAUSE()
{
	asm volatile("yield\n");
}

VOID pantheon::CPU::HLT()
{
	asm volatile("wfi\n");
}

BOOL pantheon::CPU::IF()
{
	volatile UINT64 Number = (pantheon::arm::DAIFR() >> 6);
	return (Number & 0x03) != 0;
}

VOID pantheon::CPU::LIDT(void *Table)
{
	pantheon::arm::LoadInterruptTable(Table);
}