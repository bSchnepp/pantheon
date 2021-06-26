#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>


UINT8 pantheon::CPU::GetProcessorNumber()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(RetVal) ::);
	return (UINT8)(RetVal & 0xFF);
}

VOID pantheon::CPU::CLI()
{
	pantheon::arm::CLI();
}

VOID pantheon::CPU::STI()
{
	pantheon::arm::STI();
}

VOID pantheon::CPU::PAUSE()
{
	asm volatile("yield\n");
}

extern "C" 
void enable_interrupts()
{
	pantheon::CPU::STI();
}