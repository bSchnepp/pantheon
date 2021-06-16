#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_proc.hpp"
#include "kern_sched.hpp"
#include "kern_thread.hpp"

/**
 * @file Common/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and
 * algorithms. The pantheon kernel implements a basic round-robin style scheduling
 * algorithm based on tick counts and a list of threads.
 * \~english @author Brian Schnepp
 */

pantheon::Scheduler::Scheduler()
{
	this->CurThread = 0;
}

pantheon::Scheduler::~Scheduler()
{

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
	/* Disable the system timer while a process 
	 * might need to be scheduled. 
	 */
	pantheon::DisableSystemTimer();


	/* Just before a process is restarted, make sure it's set 
	 * to use a 1000Mhz clock. This is to preempt processes as often as
	 * reasonable, and ensure the system feels low latency.
	 * (Contrast to say, Linux at 250Mhz which prioritizes throughput.)
	 * 
	 * Likewise, since we just rescheduled (forcefully or otherwise),
	 * note that we shouldn't interrupt again...
	 */
	pantheon::RearmSystemTimer();
	this->ShouldReschedule.Store(FALSE);
	/* NYI */
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	/* NYI */
	return nullptr;
}

pantheon::Thread *pantheon::Scheduler::MyThread()
{
	/* NYI */
	return nullptr;
}

void pantheon::Scheduler::MaybeReschedule()
{
	if (this->ShouldReschedule.Load() == TRUE)
	{
		/* TODO: Decrement when tick time isn't up yet. */
		this->Reschedule();
	}
}

void pantheon::Scheduler::SignalReschedule()
{
	this->ShouldReschedule.Store(TRUE);
}

pantheon::GlobalScheduler::GlobalScheduler()
{
	/* NYI */
}

pantheon::GlobalScheduler::~GlobalScheduler()
{
	/* NYI */
}

BOOL pantheon::GlobalScheduler::CreateProcess(pantheon::String ProcStr, void *StartAddr)
{
	pantheon::Process NewProc(ProcStr);
	BOOL Val = NewProc.CreateThread(StartAddr, nullptr);
	if (!Val)
	{
		return FALSE;
	}
	this->ProcessList.Add(NewProc);
	return TRUE;
}

pantheon::Process pantheon::GlobalScheduler::AcquireProcess()
{
	for (UINT64 Index = 0; Index < this->ProcessList.Size(); ++Index)
	{
		if (this->ProcessList[Index].NumInactiveThreads())
		{
			return this->ProcessList[Index];
		}
	}
	return pantheon::Process("idle");
}

void pantheon::GlobalScheduler::LoadProcess(pantheon::Process &Proc)
{
	this->ProcessList.Add(Proc);
}


static pantheon::Spinlock ThreadIDLock;
static pantheon::Spinlock ProcIDLock;
static pantheon::GlobalScheduler GlobalSched;

UINT32 pantheon::AcquireProcessID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT32 RetVal = 0;
	static UINT32 ProcessID = 0;
	ProcIDLock.Acquire();
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ProcessID++;
	ProcIDLock.Release();
	return RetVal;
}

UINT64 pantheon::AcquireThreadID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT32 RetVal = 0;
	static UINT64 ThreadID = 0;
	ThreadIDLock.Acquire();
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ThreadID++;
	ThreadIDLock.Release();
	return RetVal;
}

pantheon::GlobalScheduler *pantheon::GetGlobalScheduler()
{
	return &GlobalSched;
}