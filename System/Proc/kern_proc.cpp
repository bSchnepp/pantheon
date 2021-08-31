#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_proc.hpp"
#include "kern_sched.hpp"
#include "kern_thread.hpp"


pantheon::Process::Process()
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_VERYLOW;
	this->ProcessCommand = "idle";
	this->PID = 0;
}

pantheon::Process::Process(const char *CommandString)
{
	this->ProcessCommand = pantheon::String(CommandString);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->PID = pantheon::AcquireProcessID();
}

pantheon::Process::Process(pantheon::String &CommandString)
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
}

pantheon::Process::Process(const Process &Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
}

pantheon::Process::Process(Process &&Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
}

pantheon::Process::~Process()
{

}

pantheon::Process &pantheon::Process::operator=(const pantheon::Process &Other)
{
	if (this == &Other)
	{
		return *this;
	}
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	return *this;
}

pantheon::Process &pantheon::Process::operator=(pantheon::Process &&Other) noexcept
{
	if (this == &Other)
	{
		return *this;
	}
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	return *this;
}

[[nodiscard]]
const pantheon::String &pantheon::Process::GetProcessString() const
{
	return this->ProcessCommand;
}

[[nodiscard]] 
UINT64 pantheon::Process::NumThreads() const
{
	ArrayList<Thread> &List = pantheon::GetGlobalScheduler()->BorrowThreadList();
	UINT64 Count = 0;
	for (auto &Item : List)
	{
		if (Item.MyProc() == this)
		{
			Count++;
		}
	}
	pantheon::GetGlobalScheduler()->ReleaseThreadList();
	return Count;
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	pantheon::Thread T(this);

	/* Attempt 128KB of stack space for now... */
	UINT64 StackSz = 4096 * 4096;
	Optional<void*> StackSpace = BasicMalloc(StackSz);
	if (StackSpace.GetOkay())
	{
		UINT64 IStartAddr = (UINT64)StartAddr;
		UINT64 IThreadData = (UINT64)ThreadData;
		UINT64 IStackSpace = (UINT64)StackSpace();
		IStackSpace += StackSz;

		pantheon::CpuContext &Regs = T.GetRegisters();
		Regs.SetInitContext(IStartAddr, IThreadData, IStackSpace);

		ArrayList<Thread> &List = pantheon::GetGlobalScheduler()->BorrowThreadList();
		List.Add(T);
		pantheon::GetGlobalScheduler()->ReleaseThreadList();
	}
	return StackSpace.GetOkay();
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	return this->PID;
}

[[nodiscard]] 
pantheon::ProcessState pantheon::Process::MyState() const
{
	return this->CurState;
}

void pantheon::Process::SetState(pantheon::ProcessState State)
{
	this->CurState = State;
}