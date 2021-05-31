#include <arch.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_

namespace pantheon
{

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Thread
{
public:
	Thread();
	~Thread();

private:
	UINT64 PreemptCount;
};

typedef enum ProcessState
{
	PROCESS_STATE_INIT,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_BLOCKED,
	PROCESS_STATE_TERMINATED,
}ProcessState;

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Process
{
public:
	Process();
	~Process();

private:
	ProcessState CurState;
	ArrayList<Thread> Threads;
};

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Scheduler
{

public:
	Scheduler();
	~Scheduler();

	void Reschedule();
	Process *MyProc();

private:
	UINT64 CurProc; 
	ArrayList<Process*> Processes;
};

}

#endif