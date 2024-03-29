#include <kern.h>
#include <arch.hpp>
#include "kern_cpu.hpp"

#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <Boot/Boot.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

#include <System/Memory/kern_alloc.hpp>

/* Avoid having too high a number of cores to look through. */
static pantheon::CPU::CoreInfo PerCoreInfo[MAX_NUM_CPUS];

pantheon::CPU::CoreInfo *pantheon::CPU::GetCoreInfo()
{
	return &(PerCoreInfo[pantheon::CPU::GetProcessorNumber()]);
}

/**
 * \~english @brief Initializes a CoreInfo structure.
 * \~english @details Prepares a CoreInfo struct by initializing its
 * basic variables to an idle state, signalling that it is ready to have
 * a scheduler assigned to it to begin processing threads.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::CPU::InitCoreInfo(UINT8 CoreNo)
{
	static pantheon::Scheduler Scheds[MAX_NUM_CPUS];
	ClearBuffer((CHAR*)&PerCoreInfo[CoreNo], sizeof(pantheon::CPU::CoreInfo));
	PerCoreInfo[CoreNo].CurSched = &Scheds[CoreNo];
	(*PerCoreInfo[CoreNo].CurSched) = pantheon::Scheduler();
}

pantheon::Thread *pantheon::CPU::GetCurThread()
{
	pantheon::Scheduler *Sched = pantheon::CPU::GetCurSched();
	if (Sched == nullptr)
	{
		return nullptr;
	}
	return Sched->MyThread();
}

pantheon::Process *pantheon::CPU::GetCurProcess()
{
	pantheon::Scheduler *Sched = pantheon::CPU::GetCurSched();
	if (Sched == nullptr)
	{
		return nullptr;
	}
	pantheon::Thread *CurThread = Sched->MyThread();
	if (CurThread)
	{
		return CurThread->MyProc();
	}
	return nullptr;
}

pantheon::Scheduler *pantheon::CPU::GetCurSched()
{
	return pantheon::CPU::GetCoreInfo()->CurSched;
}

pantheon::TrapFrame *pantheon::CPU::GetCurFrame()
{
	return pantheon::CPU::GetCoreInfo()->CurFrame;
}

void *pantheon::CPU::GetStackArea(UINT64 Core)
{
	alignas(4096) static char StackArea[MAX_NUM_CPUS * DEFAULT_STACK_SIZE];
	return StackArea + static_cast<UINT64>(Core * DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE;
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

UINT64 pantheon::CPU::ICOUNT()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	return CoreInfo->NOff;
}