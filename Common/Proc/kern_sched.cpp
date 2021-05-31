#include <kern_datatypes.hpp>

#include "kern_sched.hpp"

pantheon::Thread::Thread()
{

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
 * \~english @brief Changes the currently active process of this core.
 * \~english @details Reschedules the current processor core to use a thread
 * from some other process on it's list of processes. The general relationship
 * for the scheduler is that one process is assigned to multiple different
 * schedulers, each of which run precisely one thread at a time.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::Reschedule()
{
	this->CurProc++;
	this->CurProc %= this->Processes.Size();
	/* exec that process... TODO! */
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	UINT64 ProcCount = this->Processes.Size();
	if (ProcCount == 0 || this->CurProc > ProcCount)
	{
		return nullptr;
	}
	return this->Processes[this->CurProc].GetValue();
}