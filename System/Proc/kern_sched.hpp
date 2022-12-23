#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <Sync/kern_lockable.hpp>
#include <System/Proc/kern_proc.hpp>

#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_slab.hpp>

#include <Common/Structures/kern_skiplist.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_

namespace pantheon
{

class LocalScheduler : public pantheon::Allocatable<LocalScheduler, 256>, public pantheon::Lockable
{

public:
	LocalScheduler();
	~LocalScheduler() override = default;
	UINT64 CountThreads(UINT64 PID);

	[[nodiscard]] UINT64 BusyRatio() const { return this->Threads.Size() / this->LocalRunQueue.Size(); }

	pantheon::Thread *AcquireThread();
	void InsertThread(pantheon::Thread *Thr);

	void Setup();
	pantheon::Thread *Idle();

private:
	pantheon::SkipList<UINT64, pantheon::Thread*> LocalRunQueue;
	pantheon::LinkedList<pantheon::Thread> Threads;

	pantheon::Thread *IdleThread;
};

namespace Scheduler
{
	void Init();
	BOOL SetState(UINT32 PID, pantheon::Process::State State);
	BOOL SetThreadState(UINT64 TID, pantheon::Thread::State State);
	BOOL MapPages(UINT32 PID, const pantheon::vmm::VirtualAddress *VAddresses, const pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

	UINT32 CreateProcess(const pantheon::String &ProcStr, void *StartAddr);
	pantheon::Thread *CreateThread(UINT32 Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);

	UINT64 CountThreads(UINT64 PID);

	void Lock();
	void Unlock();
	void Reschedule();

	UINT32 AcquireProcessID();
	UINT64 AcquireThreadID();

	void AttemptReschedule();
};

class ScopedGlobalSchedulerLock
{
public:
	FORCE_INLINE ScopedGlobalSchedulerLock() { pantheon::Scheduler::Lock(); }
	FORCE_INLINE ~ScopedGlobalSchedulerLock() { pantheon::Scheduler::Unlock(); }	
};

class ScopedLocalSchedulerLock
{
public:
	FORCE_INLINE ScopedLocalSchedulerLock() { pantheon::CPU::GetCurThread()->BlockScheduling(); }
	FORCE_INLINE ~ScopedLocalSchedulerLock() { pantheon::CPU::GetCurThread()->EnableScheduling(); }	
};

}

#endif