#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <Common/Structures/kern_slab.hpp>


pantheon::Process::Process()
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_VERYLOW;
	this->ProcessCommand = "idle";
	this->PID = 0;

	this->MemoryMap = nullptr;
	
}

pantheon::Process::Process(const char *CommandString)
{
	this->ProcessCommand = pantheon::String(CommandString);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->PID = pantheon::AcquireProcessID();
	
	this->MemoryMap = nullptr;
	
}

pantheon::Process::Process(pantheon::String &CommandString)
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
	
	this->MemoryMap = nullptr;
	
}

pantheon::Process::Process(const Process &Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
}

pantheon::Process::Process(Process &&Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
	ClearBuffer((CHAR*)&Other, sizeof(Process));	
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
	this->MemoryMap = Other.MemoryMap;
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
	this->MemoryMap = Other.MemoryMap;
	ClearBuffer((CHAR*)&Other, sizeof(Process));
	return *this;
}

[[nodiscard]]
const pantheon::String &pantheon::Process::GetProcessString() const
{
	return this->ProcessCommand;
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	return this->CreateThread(StartAddr, ThreadData, pantheon::THREAD_PRIORITY_NORMAL);
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	if (this->CurState == pantheon::PROCESS_STATE_INIT)
	{
		this->SetState(pantheon::PROCESS_STATE_RUNNING);
	}
	BOOL Status = pantheon::GetGlobalScheduler()->CreateThread(this, StartAddr, ThreadData, Priority);
	return Status;
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