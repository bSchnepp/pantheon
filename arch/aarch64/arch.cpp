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

BOOL pantheon::CPU::IF()
{
	return ((pantheon::arm::DAIFR() >> 6) & 0b111) != 0;
}

VOID pantheon::CPU::PUSHI()
{
	BOOL InterruptsOn = pantheon::CPU::IF();
	pantheon::CPU::CLI();

	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	if (CoreInfo->NOff == 0)
	{
		CoreInfo->IntStatus = InterruptsOn;
	}
	CoreInfo->NOff++;
}

VOID pantheon::CPU::POPI()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	if (pantheon::CPU::IF() || CoreInfo->NOff == 0)
	{
		/* This is probably an error... */
		return;
	}
	CoreInfo->NOff--;
	UINT64 NewOff = CoreInfo->NOff;
	if (CoreInfo->IntStatus && NewOff == 0)
	{
		pantheon::CPU::STI();
	}
}