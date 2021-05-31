#include <arch.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_


namespace pantheon
{

typedef enum ThreadState
{
	THREAD_STATE_INIT,
	THREAD_STATE_RUNNING,
	THREAD_STATE_BLOCKED,
	THREAD_STATE_TERMINATED,
}ThreadState;

typedef enum ThreadPriority
{
	THREAD_PRIORITY_VERYLOW = 0,
	THREAD_PRIORITY_LOW = 1,
	THREAD_PRIORITY_NORMAL = 2,
	THREAD_PRIORITY_HIGH = 3,
	THREAD_PRIORITY_VERYHIGH = 4,
}ThreadPriority;

class Process;

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Thread
{
public:
	Thread(Process *ParentProcess);
	~Thread();

private:
	CpuContext Registers;
	Process *ParentProcess;

	ThreadState State;
	ThreadPriority Priority;

	UINT64 PreemptCount;
	UINT64 RemainingTicks;
};

typedef enum ProcessState
{
	PROCESS_STATE_INIT,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_BLOCKED,
	PROCESS_STATE_TERMINATED,
}ProcessState;

typedef enum ProcessPriority
{
	PROCESS_PRIORITY_VERYLOW = 0,
	PROCESS_PRIORITY_LOW = 1,
	PROCESS_PRIORITY_NORMAL = 2,
	PROCESS_PRIORITY_HIGH = 3,
	PROCESS_PRIORITY_VERYHIGH = 4,
}ProcessPriority;

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Process
{
public:
	Process();
	~Process();

private:
	ProcessState CurState;
	ProcessPriority Priority;
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
	Thread *MyThread();

private:
	UINT64 CurThread; 
	ArrayList<Thread> Threads;
};

}

#endif