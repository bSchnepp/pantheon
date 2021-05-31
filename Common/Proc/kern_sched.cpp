#include <kern_datatypes.hpp>

#include "kern_sched.hpp"

pantheon::Thread::Thread(Process *ParentProcess)
{
	this->ParentProcess = ParentProcess;
}

pantheon::Thread::~Thread()
{

}

pantheon::Process::Process()
{

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