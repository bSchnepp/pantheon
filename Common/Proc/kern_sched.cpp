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

/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::Scheduler::Scheduler()
{
	this->CurThread = nullptr;
	this->ShouldReschedule.Store(FALSE);
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

	/* TODO: Save old registers */
	this->CurThread = pantheon::GetGlobalScheduler()->AcquireThread();


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

/**
 * \~english @brief Attempts to reschedule if the appropriate flag is fired
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::MaybeReschedule()
{
	if (this->ShouldReschedule.Load() == TRUE)
	{
		/* TODO: Decrement when tick time isn't up yet. */
		this->Reschedule();
	}
}

/**
 * \~english @brief Signals that this scheduler should run a different thread
 * \~english @author Brian Schnepp
 */
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

VOID pantheon::GlobalScheduler::CreateIdleProc(void *StartAddr)
{
	for (pantheon::Process &Proc : this->ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			Proc.CreateThread(StartAddr, nullptr);
			return;
		}
	}

	/* HACK: Until we get .init_array working, this needs to be done. */
	this->ProcessList = ArrayList<Process>(20);
	this->ThreadList = ArrayList<Thread>(50);

	pantheon::Process IdleProc;
	if (IdleProc.CreateThread(StartAddr, nullptr))
	{
		this->ProcessList.Add(IdleProc);
	}
}

static pantheon::Spinlock AcqProcSpinlock;

/**
 * \~english @brief Obtains a Process which has an inactive thread, or the idle process if none available.
 * \~english @return An instance of a Process to execute
 * \~english @author Brian Schnepp
 */
pantheon::Thread *pantheon::GlobalScheduler::AcquireThread()
{
	AcqProcSpinlock.Acquire();

	/* This should be replaced with a skiplist (or linked list), 
	 * so finding an inactive thread becomes an O(1) process.
	 */
	for (UINT64 Index = 0; Index < this->ProcessList.Size(); ++Index)
	{
		if (this->ProcessList[Index].NumInactiveThreads())
		{
			pantheon::Process *SelectedProc = &this->ProcessList[Index];
			pantheon::Thread *SelectedThread = SelectedProc->ActivateThread();
			AcqProcSpinlock.Release();
			return SelectedThread;
		}
	}
	AcqProcSpinlock.Release();
	return nullptr;
}

void pantheon::GlobalScheduler::ReleaseThread(Thread *T)
{
	AcqProcSpinlock.Acquire();
	T->MyProc()->DeactivateThread(T);
	AcqProcSpinlock.Release();
}


static pantheon::Spinlock ThreadIDLock;
static pantheon::Spinlock ProcIDLock;
static pantheon::GlobalScheduler GlobalSched;

UINT32 pantheon::AcquireProcessID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 * 
	 * 0 should be reserved for the generic idle process.
	 */
	UINT32 RetVal = 0;
	static UINT32 ProcessID = 1;
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