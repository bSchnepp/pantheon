#include <stddef.h>

#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_cpu.hpp"
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

extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset);

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
	/* Don't allow interrupts while a process is getting scheduled */
	pantheon::CPU::CLI();

	pantheon::Thread *Old = this->CurThread;
	pantheon::Thread *New = pantheon::GetGlobalScheduler()->AcquireThread();

	this->CurThread = New;
	pantheon::CPU::GetCoreInfo()->CurThread = this->CurThread;
	if (Old && New && Old != New)
	{
		pantheon::CpuContext *Prev = &(Old->GetRegisters());
		pantheon::CpuContext *Next = &(New->GetRegisters());

		this->ShouldReschedule.Store(FALSE);

		New->RefreshTicks();
		cpu_switch(Prev, Next, CpuIRegOffset);
		pantheon::GetGlobalScheduler()->ReleaseThread(Old);
	}
	pantheon::CPU::STI();
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	if (this->CurThread)
	{
		return this->CurThread->MyProc();
	}
	return nullptr;
}

pantheon::Thread *pantheon::Scheduler::MyThread()
{
	return this->CurThread;
}

/**
 * \~english @brief Attempts to reschedule if the appropriate flag is fired
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::MaybeReschedule()
{
	if (this->ShouldReschedule.Load() == TRUE)
	{
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
	this->Init();
}

pantheon::GlobalScheduler::~GlobalScheduler()
{
	/* NYI */
}

static pantheon::Spinlock ProcAccessSpinlock;

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
	pantheon::CPU::CLI();
	pantheon::Process NewProc(ProcStr);
	BOOL Val = NewProc.CreateThread(StartAddr, nullptr);

	if (Val)
	{
		ProcAccessSpinlock.Acquire();
		this->ProcessList.Add(NewProc);
		ProcAccessSpinlock.Release();
	}

	pantheon::CPU::STI();
	return Val;
}


VOID pantheon::GlobalScheduler::Init()
{
	this->ProcessList = ArrayList<Process>();
	this->ProcessList.Add(pantheon::Process());
}

VOID pantheon::GlobalScheduler::CreateIdleProc(void *StartAddr)
{
	pantheon::CPU::CLI();
	ProcAccessSpinlock.Acquire();
	for (pantheon::Process &Proc : this->ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			Proc.CreateThread(StartAddr, nullptr);
			break;
		}
	}
	ProcAccessSpinlock.Release();
	pantheon::CPU::STI();
}

/**
 * \~english @brief Obtains a Process which has an inactive thread, or the idle process if none available.
 * \~english @return An instance of a Process to execute
 * \~english @author Brian Schnepp
 */
pantheon::Thread *pantheon::GlobalScheduler::AcquireThread()
{
	pantheon::Thread *SelectedThread = nullptr;

	/* This should be replaced with a skiplist (or linked list), 
	 * so finding an inactive thread becomes an O(1) process.
	 */
	ProcAccessSpinlock.Acquire();
	for (pantheon::Process &SelectedProc : this->ProcessList)
	{
		pantheon::ProcessState PState = SelectedProc.MyState();
		if (PState == pantheon::PROCESS_STATE_TERMINATED
			|| PState == pantheon::PROCESS_STATE_ZOMBIE)
		{
			continue;
		}
		
		if (SelectedProc.NumInactiveThreads())
		{
			SelectedThread = SelectedProc.ActivateThread();
			if (SelectedThread)
			{
				break;
			}
		}
	}

	if (SelectedThread == nullptr)
	{
		for (pantheon::Process &Proc : this->ProcessList)
		{
			Proc.WipeVisited();
		}
	}

	ProcAccessSpinlock.Release();
	return SelectedThread;
}

void pantheon::GlobalScheduler::ReleaseThread(Thread *T)
{
	ProcAccessSpinlock.Acquire();
	T->MyProc()->DeactivateThread(T);
	ProcAccessSpinlock.Release();
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