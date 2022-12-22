#include <kern.h>
#include <stddef.h>

#include <arch.hpp>
#include <sync.hpp>
#include <cpureg.hpp>
#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_cpu.hpp>
#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>
#include <System/Memory/kern_alloc.hpp>

#include <Common/Structures/kern_optional.hpp>
#include <Common/Structures/kern_skiplist.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

/**
 * @file Common/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and algorithms.
 * \~english @author Brian Schnepp
 */


/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::LocalScheduler::LocalScheduler() : pantheon::Lockable("Scheduler")
{
}

/* Provided by the arch/ subdir. These differ between hardware platforms. */
extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT64 RegOffset);
extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);


UINT64 pantheon::LocalScheduler::CountThreads(UINT64 PID)
{
	UINT64 Res = 0;
	pantheon::ScopedLock _L(this);
	for (const auto &Item : this->Threads)
	{
		Res += 1ULL * (Item.MyProc()->ProcessID() == PID);
	}
	return Res;
}

pantheon::Thread *pantheon::LocalScheduler::AcquireThread()
{
	pantheon::ScopedLock _L(this);
	Optional<UINT64> LowestKey = this->LocalRunQueue.MinKey();
	if (!LowestKey.GetOkay())
	{
		return nullptr;
	}

	Optional<pantheon::Thread*> Lowest = this->LocalRunQueue.Get(LowestKey.GetValue());
	if (!Lowest.GetOkay())
	{
		return nullptr;
	}

	this->LocalRunQueue.Delete(LowestKey.GetValue());
	return Lowest.GetValue();

}

static UINT64 CalculateDeadline(UINT64 Jiffies, UINT64 PrioRatio, UINT64 RRInterval)
{
	return Jiffies + (PrioRatio * RRInterval);
}

/**
 * \~english @brief Inserts a thread for this local scheduler
 * \~english @param Thr The thread object to insert. Must be locked before calling.
 */
void pantheon::LocalScheduler::InsertThread(pantheon::Thread *Thr)
{
	pantheon::ScopedLock _L(this);
	this->Threads.PushFront(Thr);
	if (Thr->MyState() == pantheon::Thread::STATE_WAITING)
	{
		UINT64 Jiffies = pantheon::CPU::GetJiffies();
		UINT64 Prio = pantheon::Thread::PRIORITY_MAX - Thr->MyPriority();
		this->LocalRunQueue.Insert(CalculateDeadline(Jiffies, Prio, 6), Thr);
	}
}

pantheon::Spinlock SchedLock("Global Scheduler Lock");

void pantheon::Scheduler::Lock()
{
	SchedLock.Acquire();
}

void pantheon::Scheduler::Unlock()
{
	SchedLock.Release();
}

/**
 * \~english @brief Creates a process, visible globally, from a name and address.
 * \~english @details A process is created, such that it can be run on any
 * available processor on the system. The given process name identifies the
 * process for debugging purposes and as a user-readable way to identify a
 * process, and the start address is a pointer to the first instruction the
 * initial threaq should execute when scheduled.
 * \~english @param[in] ProcStr A human-readable name for the process
 * \~english @param[in] StartAddr The initial value of the program counter
 * \~english @return TRUE is the process was sucessfully created, false otherwise.
 * \~english @author Brian Schnepp
 */
UINT32 pantheon::Scheduler::CreateProcess(const pantheon::String &ProcStr, void *StartAddr)
{
	PANTHEON_UNUSED(ProcStr);
	PANTHEON_UNUSED(StartAddr);
	return 0;
}

pantheon::Thread *pantheon::Scheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	PANTHEON_UNUSED(Proc);
	PANTHEON_UNUSED(StartAddr);
	PANTHEON_UNUSED(ThreadData);
	PANTHEON_UNUSED(Priority);
	return nullptr;
}


UINT64 pantheon::Scheduler::CountThreads(UINT64 PID)
{
	UINT64 Count = 0;
	PANTHEON_UNUSED(PID);
	return Count;
}

UINT32 pantheon::Scheduler::AcquireProcessID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 * 
	 * 0 should be reserved for the generic idle process.
	 */
	UINT32 RetVal = 0;
	static UINT32 ProcessID = 1;
	RetVal = ProcessID++;
	return RetVal;
}

UINT64 pantheon::Scheduler::AcquireThreadID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT32 RetVal = 0;
	static UINT64 ThreadID = 0;
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ThreadID++;
	return RetVal;
}

/**
 * \~english @brief Changes the current thread of this core.
 * \~english @details Forcefully changes the current thread by iterating to
 * the next thread in the list of threads belonging to this core. The active
 * count for the current thread is set to zero, and the new thread is run until
 * either the core is forcefully rescheduled, or the timer indicates the current
 * thread should quit.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::Reschedule()
{

}

void pantheon::Scheduler::AttemptReschedule()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();

	if (CurThread)
	{
		CurThread->Lock();
		CurThread->CountTick();
		UINT64 RemainingTicks = CurThread->TicksLeft();

		if (RemainingTicks > 0 || CurThread->Preempted())
		{
			CurThread->Unlock();
			return;
		}
		
		CurThread->SetTicks(0);
		CurThread->Unlock();


		if (pantheon::CPU::ICOUNT() != 0)
		{
			StopError("Interrupts not allowed for reschedule");
		}

		pantheon::Scheduler::Reschedule();
	}
}

extern "C" VOID FinishThread()
{
	pantheon::CPU::GetCurThread()->EnableScheduling();
}

BOOL pantheon::Scheduler::MapPages(UINT32 PID, const pantheon::vmm::VirtualAddress *VAddresses, const pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages)
{
	BOOL Success = FALSE;
	PANTHEON_UNUSED(PID);
	PANTHEON_UNUSED(VAddresses);
	PANTHEON_UNUSED(PAddresses);
	PANTHEON_UNUSED(PageAttributes);
	PANTHEON_UNUSED(NumPages);
	return Success;
}

BOOL pantheon::Scheduler::RunProcess(UINT32 PID)
{
	BOOL Success = FALSE;
	PANTHEON_UNUSED(PID);
	return Success;	
}

BOOL pantheon::Scheduler::SetState(UINT32 PID, pantheon::Process::State State)
{
	BOOL Success = FALSE;
	PANTHEON_UNUSED(PID);
	PANTHEON_UNUSED(State);
	return Success;	
}
