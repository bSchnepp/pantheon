#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_sched.hpp"

/**
 * @file Common/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and
 * algorithms. The pantheon kernel implements a basic round-robin style scheduling
 * algorithm based on tick counts and a list of threads.
 * \~english @author Brian Schnepp
 */


/**
 * \~english @brief Initializes a new thread belonging to a process.
 * \~english @author Brian Schnepp
 * \~english @param OwningProcess A pointer to the owning process for which to
 * register the current thread.
 */
pantheon::Thread::Thread(Process *OwningProcess) 
	: pantheon::Thread::Thread(OwningProcess, pantheon::THREAD_PRIORITY_NORMAL)
{
}

/**
 * \~english @brief Initializes a new thread belonging to a process.
 * \~english @author Brian Schnepp
 * \~english @param OwningProcess A pointer to the owning process for which to
 * register the current thread.
 * \~english @param Priority The priority of the current thread. The thread
 * priority given will never be greater than the supplied priority. 
 */
pantheon::Thread::Thread(Process *OwningProcess, ThreadPriority Priority)
{
	this->ParentProcess = OwningProcess;
	this->Registers.Wipe();

	this->State = pantheon::THREAD_STATE_INIT;
	this->Priority = Priority;

	this->PreemptCount = 0;
	this->RemainingTicks = 0;

	/* 45 for NORMAL, 30 for LOW, 15 for VERYLOW, etc. */
	this->AddTicks((Priority + 1) * 15);
}

pantheon::Thread::Thread(const pantheon::Thread &Other)
{
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
}

pantheon::Thread::~Thread()
{

}

/**
 * \~english @brief Obtains a handle to the process that owns this thread
 * \~english @author Brian Schnepp
 * \~english @return A pointer to the owning process of this thread.
 */
pantheon::Process *pantheon::Thread::MyProc()
{
	return this->ParentProcess;
}

/**
 * \~english @brief Gets the state, such as running or blocked, of this thread.
 * \~english @author Brian Schnepp
 * \~english @return The current status of this thread
 */
pantheon::ThreadState pantheon::Thread::MyState()
{
	return this->State;
}

/**
 * \~english @brief Gets the priority of the thread, used for time allocation.
 * \~english @author Brian Schnepp
 * \~english @return The current priority of this thread
 */
[[nodiscard]]
pantheon::ThreadPriority pantheon::Thread::MyPriority()
{
	return this->Priority;
}

/**
 * \~english @brief Gets the number of times this process has been pre-empted.
 * \~english @author Brian Schnepp
 * \~english @return The number of times the kernel has interrupted this thread
 * to resume execution of some other work.
 */
[[nodiscard]]
UINT64 pantheon::Thread::Preempts() const
{
	return this->PreemptCount;
}

/**
 * \~english @brief Gets the number of timer interrupts remaining for the thread
 * \~english @author Brian Schnepp
 * \~english @return The number of times the processor core will need to be
 * interrupted before the thread is preempted and some other work resumes on
 * the current processor core.
 */
UINT64 pantheon::Thread::TicksLeft() const
{
	return this->RemainingTicks;
}

/**
 * \~english @brief Forcefully adds execution time to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::AddTicks(UINT64 TickCount)
{
	this->RemainingTicks += TickCount;
}

/**
 * \~english @brief Sets the state, such as running, to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetState(ThreadState State)
{
	this->State = State;
}

/**
 * \~english @brief Changes the priority of the current thread
 * \~english @details Informs the kernel that this thread should be scheduled
 * less often as compared to others. This generally happens for something such
 * as a network services daemon, which does not need to run too often.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetPriority(ThreadPriority Priority)
{
	if (Priority <= this->Priority)
	{
		this->Priority = Priority;
	}
}

/**
 * \~english @brief Obtains a reference to the registers of the thread.
 * \~english @author Brian Schnepp
 */
pantheon::CpuContext &pantheon::Thread::GetRegisters()
{
	/* TODO: Copy the actual registers to the internal representation! */
	return this->Registers;
}

pantheon::Thread &pantheon::Thread::operator=(const pantheon::Thread &Other)
{
	if (this == &Other)
	{
		return *this;
	}
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	return *this;
}

pantheon::Process::Process() : pantheon::Process::Process("kernel")
{
}

pantheon::Process::Process(const char *CommandString)
{
	this->ProcessCommand = pantheon::String(CommandString);
	*this = Process(this->ProcessCommand);
}

pantheon::Process::Process(pantheon::String &CommandString)
{
	PANTHEON_UNUSED(CurState);
	PANTHEON_UNUSED(Priority);
	this->ProcessCommand = CommandString;
	this->ProcessID = pantheon::AcquireProcessID();
}

pantheon::Process::~Process()
{

}

[[nodiscard]]
const pantheon::String &pantheon::Process::GetProcessString() const
{
	return this->ProcessCommand;
}

[[nodiscard]] 
UINT64 pantheon::Process::NumThreads() const
{
	return this->Threads.Size();
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	pantheon::Thread T(this);
	/* Attempt 4KB of stack space for now... */
	Optional<void*> StackSpace = BasicMalloc(4 * 1024);
	if (StackSpace.GetOkay() == FALSE)
	{
		return FALSE;
	}

	pantheon::CpuContext &Regs = T.GetRegisters();
	Regs.SetPC((UINT64)(StartAddr));
	Regs.SetSP((UINT64)StackSpace.GetValue());
	Regs.SetArg1((UINT64)ThreadData);
	this->Threads.Add(T);
	return TRUE;
}

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

void pantheon::GlobalScheduler::CreateProcess(pantheon::String ProcStr, void *StartAddr)
{
	pantheon::Process NewProc(ProcStr);
	NewProc.CreateThread(StartAddr, nullptr);
	this->ProcessesWithInactiveThreads.Add(NewProc);
}

Optional<pantheon::Process> pantheon::GlobalScheduler::AcquireProcess()
{
	if (this->ProcessesWithInactiveThreads.Size() == 0)
	{
		return Optional<pantheon::Process>();
	}
	return Optional<Process>(this->ProcessesWithInactiveThreads[0]);
}


static pantheon::Spinlock ProcIDLock;

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

[[nodiscard]]
UINT32 pantheon::Process::GetProcessID() const
{
	return this->ProcessID;
}