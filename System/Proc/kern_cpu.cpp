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
static UINT8 NoCPUs = 0;
static pantheon::CPU::CoreInfo PerCoreInfo[MAX_NUM_CPUS];

void pantheon::CPU::InitCore(UINT8 CoreNo)
{
	PerCoreInfo[CoreNo] = {};
	PerCoreInfo[CoreNo].LocalSched = pantheon::LocalScheduler::Create();
	NoCPUs++;

	/* This is needed since kernel constructors happen to be ordered 
	 * in a way that could cause a problem for our initialization order... 
	 */
	PerCoreInfo[CoreNo].LocalSched->Setup();
}

UINT8 pantheon::CPU::GetNoCPUs()
{
	return NoCPUs;
}

pantheon::CPU::CoreInfo *pantheon::CPU::GetCoreInfo()
{
	return &(PerCoreInfo[pantheon::CPU::GetProcessorNumber()]);
}

pantheon::Thread *pantheon::CPU::GetCurThread()
{
	return PerCoreInfo[pantheon::CPU::GetProcessorNumber()].CurThread;
}

pantheon::Process *pantheon::CPU::GetCurProcess()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (CurThread)
	{
		return CurThread->MyProc();
	}
	return nullptr;
}

UINT64 pantheon::CPU::GetJiffies()
{
	return pantheon::CPU::GetCoreInfo()->LocalJiffies;
}

pantheon::TrapFrame *pantheon::CPU::GetCurFrame()
{
	return pantheon::CPU::GetCoreInfo()->CurFrame;
}

pantheon::LocalScheduler *pantheon::CPU::GetLocalSched(UINT8 ProcNo)
{
	if (ProcNo >= pantheon::CPU::GetNoCPUs())
	{
		return nullptr;
	}
	return PerCoreInfo[ProcNo].LocalSched;
}

pantheon::LocalScheduler *pantheon::CPU::GetMyLocalSched()
{
	return pantheon::CPU::GetLocalSched(pantheon::CPU::GetProcessorNumber());
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