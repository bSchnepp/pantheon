#include <kern_datatypes.hpp>

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

pantheon::Process::Process()
{
	PANTHEON_UNUSED(CurState);
	PANTHEON_UNUSED(Priority);
}

pantheon::Process::~Process()
{

}

pantheon::Scheduler::Scheduler()
{

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
	/* NYI */
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	/* NYI */
	return nullptr;
}