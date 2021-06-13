#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>

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
	Thread(Process *ParentProcess, ThreadPriority Priority);
	Thread(const Thread &Other);
	~Thread();

	Process *MyProc();

	ThreadState MyState();
	ThreadPriority MyPriority();

	[[nodiscard]] UINT64 Preempts() const;
	[[nodiscard]] UINT64 TicksLeft() const;

	VOID AddTicks(UINT64 TickCount);

	VOID SetState(ThreadState State);
	VOID SetPriority(ThreadPriority Priority);

	VOID SetEntryLocation(UINT64 IP, UINT64 SP, VOID* ThreadData);

	CpuContext &GetRegisters();

	Thread &operator=(const Thread &Other);

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
	Process(const char *CommandString);
	Process(String &CommandString);
	~Process();

	[[nodiscard]] const String &GetProcessString() const;
	[[nodiscard]] UINT32 GetProcessID() const;

	[[nodiscard]] UINT64 NumThreads() const;
	BOOL CreateThread(void *StartAddr, void *ThreadData);

private:
	UINT32 ProcessID;
	String ProcessCommand;
	
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

	void MaybeReschedule();
	void SignalReschedule();

private:

	Atomic<BOOL> ShouldReschedule;

	UINT64 CurThread;
	ArrayList<Thread> Threads;
};

class GlobalScheduler
{

public:
	GlobalScheduler();
	~GlobalScheduler();

	void CreateProcess(pantheon::String ProcStr, void *StartAddr);

	Optional<Process> AcquireProcess();

private:
	ArrayList<Thread> InactiveThreads;
	ArrayList<Process> ProcessesWithInactiveThreads;
};

UINT32 AcquireProcessID();

}

#endif