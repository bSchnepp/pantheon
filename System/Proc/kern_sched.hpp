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

class LocalScheduler : public pantheon::Allocatable<LocalScheduler, MAX_NUM_CPUS>, public pantheon::Lockable
{

public:
	/**
	 * \~english @brief Initalizes an instance of a per-core scheduler.
	 * \~english @author Brian Schnepp
	 */
	LocalScheduler();
	~LocalScheduler() override = default;

	/**
	 * \~english @brief Counts the number of threads under this scheduler which belong to a given PID.
	 * \~english @pre This scheduler is not locked
	 * \~english @return The number of threads under this manager which belong to a given process 
	 */
	UINT64 CountThreads(UINT64 PID);

	/**
	 * \~english @brief Obtains a thread from the local runqueue which can be run, and removes it from the local runqueue.
	 * \~english @pre This scheduler is locked
	 * \~english @return A thread which can be run, if it exists. Nullptr otherwise.
	 */
	pantheon::Thread *AcquireThread();

	/**
	 * \~english @brief Inserts a thread for this local scheduler
	 * \~english @pre The thread to be inserted is locked before calling
	 * \~english @pre This scheduler is not locked
	 * \~english @param Thr The thread object to insert.
	 */
	void InsertThread(pantheon::Thread *Thr);

	/** @private */ void Setup();
	/** @private */ pantheon::Thread *Idle();

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

class ScopedRescheduleLock
{
public:
	FORCE_INLINE ScopedRescheduleLock() { pantheon::CPU::GetCurThread()->BlockScheduling(); }
	FORCE_INLINE ~ScopedRescheduleLock() { pantheon::CPU::GetCurThread()->EnableScheduling(); }	
};

}

#endif