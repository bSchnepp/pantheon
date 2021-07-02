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
	this->InactiveTIDCount = 0;
}

pantheon::Process::Process(const char *CommandString)
{
	this->ProcessCommand = pantheon::String(CommandString);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->PID = pantheon::AcquireProcessID();
	this->InactiveTIDCount = 0;
}

pantheon::Process::Process(pantheon::String &CommandString)
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
	this->InactiveTIDCount = 0;
}

pantheon::Process::Process(const Process &Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->Threads.Copy(Other.Threads);
	this->InactiveTIDCount = Other.InactiveTIDCount;
}

pantheon::Process::Process(Process &&Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->Threads.Move(Other.Threads);
	this->InactiveTIDCount = Other.InactiveTIDCount;
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
	this->Threads = Other.Threads;
	this->InactiveTIDCount = Other.InactiveTIDCount;
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
	this->Threads.Move(Other.Threads);
	this->InactiveTIDCount = Other.InactiveTIDCount;
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
	return this->Threads.Size();
}

[[nodiscard]] UINT64 pantheon::Process::DefaultThreadStackSize()
{
	return 128 * 1024;
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	pantheon::Thread T(this);

	/* Attempt 128KB of stack space for now... */
	UINT64 StackSz = pantheon::Process::DefaultThreadStackSize();
	Optional<void*> StackSpace = BasicMalloc(StackSz);
	if (StackSpace.GetOkay())
	{
		UINT64 IStartAddr = (UINT64)StartAddr;
		UINT64 IThreadData = (UINT64)ThreadData;
		UINT64 IStackSpace = (UINT64)StackSpace();
		IStackSpace += StackSz;

		pantheon::CpuContext &Regs = T.GetRegisters();
		Regs.SetInitContext(IStartAddr, IThreadData, IStackSpace);

		this->CreateThreadLock.Acquire();
		this->InactiveTIDCount++;
		this->Threads.Add(T);
		this->CreateThreadLock.Release();
	}
	return StackSpace.GetOkay();
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	return this->PID;
}


[[nodiscard]] 
UINT64 pantheon::Process::NumInactiveThreads() const
{
	return this->InactiveTIDCount;
}


static pantheon::Spinlock ActivateThreadLock;
pantheon::Thread* pantheon::Process::ActivateThread()
{
	if (this->NumInactiveThreads() == 0)
	{
		return nullptr;
	}

	ActivateThreadLock.Acquire();

	pantheon::Thread *SelectedThread = nullptr;

	/* TODO: Gracefully handle BLOCKED state. */
	for (pantheon::Thread &T : this->Threads)
	{
		pantheon::ThreadState TState = T.MyState();
		if (TState == pantheon::THREAD_STATE_RUNNING 
			|| TState == pantheon::THREAD_STATE_TERMINATED)
		{
			continue;
		}

		T.SetState(pantheon::THREAD_STATE_RUNNING);
		this->InactiveTIDCount--;
		SelectedThread = &T;
		break;
	}
	
	ActivateThreadLock.Release();
	return SelectedThread;
}

void pantheon::Process::DeactivateThread(pantheon::Thread *T)
{
	ActivateThreadLock.Acquire();
	T->SetState(pantheon::THREAD_STATE_WAITING);
	this->InactiveTIDCount++;
	ActivateThreadLock.Release();
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