#include <kern.h>
#include <arch.hpp>
#include "kern_cpu.hpp"

#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <Boot/Boot.hpp>
#include <kern_datatypes.hpp>
#include <PhyMemory/kern_alloc.hpp>

#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

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

	PerCoreInfo[CoreNo].CurFrame = nullptr;
	PerCoreInfo[CoreNo].NOff = 0;
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

extern "C" void *get_stack_area()
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	return pantheon::CPU::GetStackArea(CpuNo);
}