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
	VOID CreateIdleProc(void *StartAddr);

	Thread* AcquireThread();
	void ReleaseThread(Thread *T);

private:
	ArrayList<Process> ProcessList;
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();
GlobalScheduler *GetGlobalScheduler();

}

#endif