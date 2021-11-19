#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_

namespace pantheon
{

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
	VOID PerformCpuSwitch(Thread *Old, Thread *New);

	Thread *CurThread;
	Atomic<BOOL> ShouldReschedule;
	Atomic<BOOL> IgnoreReschedule;

	pantheon::Thread IdleThread;
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
	UINT64 CountThreads(UINT64 PID);
	void ReleaseThread(Thread *T);

	pantheon::Process *ObtainProcessByID(UINT64 PID);
	pantheon::Thread *ObtainThreadByID(UINT64 TID);

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