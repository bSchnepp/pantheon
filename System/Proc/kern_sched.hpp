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
	Thread *IdleThread;
};

class GlobalScheduler
{

public:
	static void Init();

	static UINT32 CreateProcess(const pantheon::String &ProcStr, void *StartAddr);
	static pantheon::Thread *CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);

	static pantheon::Thread *CreateUserThread(UINT32 PID, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);
	static pantheon::Thread *CreateUserThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);

	static UINT64 CountThreads(UINT64 PID);

	static pantheon::Thread *AcquireThread();
	static pantheon::Thread *CreateProcessorIdleThread();

	static BOOL RunProcess(UINT32 PID);
	static BOOL SetState(UINT32 PID, pantheon::Process::State State);
	static BOOL MapPages(UINT32 PID, pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

	static void AppendIntoReadyList(pantheon::Thread *Next);
	static void RemoveFromReadyList(pantheon::Thread *Next);
	static pantheon::Thread *PopFromReadyList();

	/* TODO: Inherit from Lockable... */
	static void Lock();
	static void Unlock();

private:
	inline static Atomic<BOOL> Okay;
	inline static Spinlock AccessSpinlock;

	inline static LinkedList<Process> ProcessList;
	inline static LinkedList<Thread> ThreadList;

	inline static Thread *ReadyHead;
	inline static Thread *ReadyTail;	

private:
	static Thread *CreateUserThreadCommon(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority);
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();

void AttemptReschedule();

}

#endif