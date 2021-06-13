#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>

#ifndef _KERN_PROC_HPP_
#define _KERN_PROC_HPP_

namespace pantheon
{

class Thread;

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
	[[nodiscard]] UINT32 ProcessID() const;

	[[nodiscard]] UINT64 NumThreads() const;
	BOOL CreateThread(void *StartAddr, void *ThreadData);

private:
	UINT32 PID;
	String ProcessCommand;
	
	ProcessState CurState;
	ProcessPriority Priority;
	ArrayList<Thread> Threads;
};

}

#endif