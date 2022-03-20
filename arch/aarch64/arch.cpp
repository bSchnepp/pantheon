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
	pantheon::Sync::ISB();
}

VOID pantheon::CPU::POPI()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	if (pantheon::CPU::IF() == FALSE || CoreInfo->NOff == 0)
	{
		/* This is probably an error... */
		StopError("Mismatched PUSHI/POPI (trying to pop)");
		return;
	}
	CoreInfo->NOff--;
	UINT64 NewOff = CoreInfo->NOff;
	if (CoreInfo->IntStatus && NewOff == 0)
	{
		pantheon::CPU::STI();
	}
	pantheon::Sync::ISB();
}

VOID pantheon::CPU::LIDT(void *Table)
{
	pantheon::arm::LoadInterruptTable(Table);
}

UINT64 pantheon::CPU::ICOUNT()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	return CoreInfo->NOff;
}