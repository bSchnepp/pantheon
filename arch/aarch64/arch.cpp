#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#include <Common/Sync/kern_atomic.hpp>


static pantheon::Atomic<BOOL> InterruptsEnabled[256];

UINT8 pantheon::CPU::GetProcessorNumber()
{
	UINT64 RetVal = 0;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(RetVal) ::);
	return (UINT8)(RetVal & 0xFF);
}

VOID pantheon::CPU::CLI()
{
	UINT8 ProcNum = pantheon::CPU::GetProcessorNumber();
	if (InterruptsEnabled[ProcNum].Load() == FALSE)
	{
		return;
	}
	pantheon::arm::CLI();
	InterruptsEnabled[ProcNum].Store(FALSE);
}

VOID pantheon::CPU::STI()
{
	UINT8 ProcNum = pantheon::CPU::GetProcessorNumber();
	if (InterruptsEnabled[ProcNum].Load() == TRUE)
	{
		return;
	}
	pantheon::arm::STI();
	InterruptsEnabled[ProcNum].Store(TRUE);
}

VOID pantheon::CPU::PAUSE()
{
	asm volatile("yield\n");
}