#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <System/Proc/kern_proc.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_

namespace pantheon
{

class Scheduler
{

public:
	Scheduler();
	~Scheduler();

	void Reschedule();
	Process *MyProc();
	Thread *MyThread();

private:
	VOID PerformCpuSwitch(Thread *Old, Thread *New);
	Thread *CurThread;
};

class GlobalScheduler
{

public:
	GlobalScheduler();
	~GlobalScheduler();

	void Init();

	UINT32 CreateProcess(pantheon::String ProcStr, void *StartAddr);
	pantheon::Thread *CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority = pantheon::THREAD_PRIORITY_NORMAL);
	pantheon::Thread *CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority, void *StackTop);

	Thread* AcquireThread();
	UINT64 CountThreads(UINT64 PID);
	void ReleaseThread(Thread *T);

	pantheon::Thread *CreateProcessorIdleThread(UINT64 SP, UINT64 IP);

	BOOL SetState(UINT32 PID, pantheon::ProcessState State);
	BOOL MapPages(UINT32 PID, pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

private:
	Atomic<BOOL> Okay;
	Spinlock AccessSpinlock;
	ArrayList<Process> ProcessList;
	ArrayList<Thread> ThreadList;
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();
GlobalScheduler *GetGlobalScheduler();

void AttemptReschedule();

}

#endif