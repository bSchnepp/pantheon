#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_proc.hpp"
#include "kern_sched.hpp"
#include "kern_thread.hpp"


pantheon::Process::Process() : pantheon::Process::Process("kernel")
{
}

pantheon::Process::Process(const char *CommandString)
{
	this->ProcessCommand = pantheon::String(CommandString);
	*this = Process(this->ProcessCommand);
}

pantheon::Process::Process(pantheon::String &CommandString)
{
	PANTHEON_UNUSED(CurState);
	PANTHEON_UNUSED(Priority);
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
}

pantheon::Process::~Process()
{

}

[[nodiscard]]
const pantheon::String &pantheon::Process::GetProcessString() const
{
	return this->ProcessCommand;
}

[[nodiscard]] 
UINT64 pantheon::Process::NumThreads() const
{
	return this->Threads.Size();
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	pantheon::Thread T(this);
	/* Attempt 4KB of stack space for now... */
	Optional<void*> StackSpace = BasicMalloc(4 * 1024);
	if (StackSpace.GetOkay() == FALSE)
	{
		return FALSE;
	}

	pantheon::CpuContext &Regs = T.GetRegisters();
	Regs.SetPC((UINT64)(StartAddr));
	Regs.SetSP((UINT64)StackSpace.GetValue());
	Regs.SetArg1((UINT64)ThreadData);
	this->Threads.Add(T);
	return TRUE;
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	return this->PID;
}
