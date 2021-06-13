#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>

#ifndef _KERN_THREAD_HPP_
#define _KERN_THREAD_HPP_

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
	[[nodiscard]] UINT64 ThreadID() const;

	VOID AddTicks(UINT64 TickCount);

	VOID SetState(ThreadState State);
	VOID SetPriority(ThreadPriority Priority);

	VOID SetEntryLocation(UINT64 IP, UINT64 SP, VOID* ThreadData);

	CpuContext &GetRegisters();

	Thread &operator=(const Thread &Other);

private:
	UINT64 TID;

	CpuContext Registers;
	Process *ParentProcess;

	ThreadState State;
	ThreadPriority Priority;

	UINT64 PreemptCount;
	UINT64 RemainingTicks;
};

}

#endif