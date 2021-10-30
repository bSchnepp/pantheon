#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_


namespace pantheon
{

/* NOT YET IMPLEMENTED! Placeholder for CPUInfo!!!! */
class Scheduler
{

public:
	Scheduler();
	~Scheduler();

	void Reschedule();
	Process *MyProc();
	Thread *MyThread();

	void MaybeReschedule();
	void SignalReschedule();

private:
	BOOL PerformCpuSwitch(Thread *Old, Thread *New);

	Thread *CurThread;
	Atomic<BOOL> ShouldReschedule;
};

class GlobalScheduler
{

public:
	GlobalScheduler();
	~GlobalScheduler();

	void Init();

	BOOL CreateProcess(pantheon::String ProcStr, void *StartAddr);
	BOOL CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority);
	BOOL CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority, void *StackTop);
	VOID CreateIdleProc(void *StartAddr);

	Thread* AcquireThread();
	UINT64 CountThreads(UINT64 TID);
	void ReleaseThread(Thread *T);

private:
	Spinlock AccessSpinlock;
	ArrayList<Process> ProcessList;
	ArrayList<Thread> ThreadList;
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();
GlobalScheduler *GetGlobalScheduler();

void AttemptReschedule();

}

#endif