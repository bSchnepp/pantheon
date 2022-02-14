#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <System/Proc/kern_proc.hpp>

#include <Common/Structures/kern_slab.hpp>
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

	static UINT32 CreateProcess(pantheon::String ProcStr, void *StartAddr);
	static pantheon::Thread *CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority = pantheon::THREAD_PRIORITY_NORMAL);
	static pantheon::Thread *CreateUserThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority = pantheon::THREAD_PRIORITY_NORMAL);

	static UINT64 CountThreads(UINT64 PID);

	static pantheon::Thread *AcquireThread();
	static pantheon::Thread *CreateProcessorIdleThread(UINT64 SP, UINT64 IP);

	static BOOL SetState(UINT32 PID, pantheon::ProcessState State);
	static BOOL MapPages(UINT32 PID, pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

private:
	inline static Atomic<BOOL> Okay;
	inline static Spinlock AccessSpinlock;

	inline static LinkedList<Process> ProcessList;
	inline static LinkedList<Thread> ThreadList;

	/* TODO: make these into SlabAllocators! */
	inline static pantheon::mm::SlabCache<Process> ProcAllocator;
	inline static pantheon::mm::SlabCache<Thread> ThreadAllocator;

	static constexpr UINT16 NumProcs = 128;
	static constexpr UINT16 NumThreads = 4 * NumProcs;

	inline static Process ArrayProcs[NumProcs];
	inline static Thread ArrayThreads[NumThreads];
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();
GlobalScheduler *GetGlobalScheduler();

void AttemptReschedule();

}

#endif