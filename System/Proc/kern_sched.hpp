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

/**
 * \~english @brief An instance of a per-core scheduler, including the local runqueue
 * \~english @author Brian Schnepp
 */
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

/**
 * \~english @brief Functions for managing and accessing processes using the global scheduler's state
 * \~english @author Brian Schnepp
 */
namespace Scheduler
{
	/** @private */ void Init();

	/**
	 * \~english @brief Sets the state of a given process
	 * \~english @pre The global scheduler lock is not held by this processor
	 * \~english @pre The process lock is not held by this processor
	 * \~english @param[in] PID The process ID to modify
	 * \~english @param[in] State The state to set the process to
	 */
	BOOL SetState(UINT32 PID, pantheon::Process::State State);

	/**
	 * \~english @brief Sets the state of a given thread
	 * \~english @pre The global scheduler lock is not held by this processor
	 * \~english @pre The thread lock is not held by this processor
	 * \~english @param[in] TID The thread ID to modify
	 * \~english @param[in] State The state to set the thread to
	 */
	BOOL SetThreadState(UINT64 TID, pantheon::Thread::State State);

	/**
	 * \~english @brief Maps some pages into a given process' address space
	 * \~english @param[in] PID The process ID to map addresses into
	 * \~english @param[in] VAddresses The set of virtual addresses to map these pages as
	 * \~english @param[in] PAddresses The set of physical addresses to map these pages from
	 * \~english @param[in] PageAttributes The set of page attributes (permissions, cachability, etc.) to use for all of these pages
	 * \~english @param[in] NumPages The number of pages to map
	 * \~english @return TRUE on success, FALSE otherwise
	 * \~english @pre Length(VAddresses) == Length(PAddress)
	 * \~english @pre Length(VAddresses) >= NumPages
	 * \~english @pre Length(PAddresses) >= NumPages
	 * \~english @author Brian Schnepp
	 */
	BOOL MapPages(UINT32 PID, const pantheon::vmm::VirtualAddress *VAddresses, const pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

	/**
	 * \~english @brief Creates a process, visible globally, from a name and address.
	 * \~english @details A process is created, such that it can be run on any
	 * available processor on the system. The given process name identifies the
	 * process for debugging purposes and as a user-readable way to identify a
	 * process, and the start address is a pointer to the first instruction the
	 * initial thread should execute when scheduled.
	 * \~english @param[in] ProcStr A human-readable name for the process
	 * \~english @param[in] StartAddr The initial value of the program counter
	 * \~english @return New PID if the process was sucessfully created, 0 otherwise.
	 * \~english @author Brian Schnepp
	 */
	UINT32 CreateProcess(const pantheon::String &ProcStr, void *StartAddr);

	/**
	 * \~english @brief Creates a process, visible globally, under a given process.
	 * \~english @details A thread is created, such that it can be run on any
	 * available processor on the system, but will typically default to the 
	 * current processor where this function is called. StartAddr is a pointer to
	 * the location of the entrypoint of the new thread, with it's first argument 
	 * being ThreadData.
	 * \~english @param[in] Proc The process to associate under
	 * \~english @param[in] StartAddr The entrypoint of the thread
	 * \~english @param[in] ThreadData The contents of the first argument register when calling StartAddr
	 * \~english @param[in] Priority The priority of the thread
	 * \~english @return The newly created ThreadID of the thread if successful, 0 otherwise.
	 * \~english @author Brian Schnepp
	 */
	UINT64 CreateThread(UINT32 Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);

	/**
	 * \~english @brief Counts the number of threads which belong to a given PID.
	 * \~english @pre Global scheduler is not locked
	 * \~english @return The number of threads which belong to a given process 
	 */
	UINT64 CountThreads(UINT64 PID);

	/**
	 * \~english @brief Locks the global scheduler, preventing operations such as thread or process modification from proceeding on other threads
	 * @see ScopedGlobalSchedulerLock
	 */
	void Lock();

	/**
	 * \~english @brief Releases the global scheduler, allowing operations such as thread or process modification to proceed on other threads
	 * @see ScopedGlobalSchedulerLock
	 */
	void Unlock();

	/**
	 * \~english @brief Changes the current thread of this core.
	 * \~english @details Forcefully changes the current thread by iterating to
	 * the next thread in the list of threads belonging to this core. The active
	 * count for the current thread is set to zero, and the new thread is run until
	 * either the core is forcefully rescheduled, or the timer indicates the current
	 * thread should quit.
	 * \~english @pre Interrupts are enabled on this processor (no locks are held)
	 * \~english @author Brian Schnepp
	 */
	void Reschedule();

	/**
	 * \~english @brief Checks for the rescheduling condition, and triggers if possible
	 * \~english @details Checks if the number of jiffies the current thread has been 
	 * allocated have expired, and if so, switches to a different context. The number of 
	 * jiffies a thread has will be determined by the constant RR_INTERVAL in the class 
	 * pantheon::Thread. This will usually be 6, so that threads have a fair amount of
	 * time, but some threads may be scheduled before others depending on the current
	 * scheduling policy.
	 * 
	 * @see pantheon::Thread
	 * 
	 * \~english @author Brian Schnepp
	 */	
	void AttemptReschedule();
};

class ScopedGlobalSchedulerLock
{
public:
	FORCE_INLINE ScopedGlobalSchedulerLock() { pantheon::Scheduler::Lock(); }
	FORCE_INLINE ~ScopedGlobalSchedulerLock() { pantheon::Scheduler::Unlock(); }	
};

}

#endif