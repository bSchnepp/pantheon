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
	this->InactiveTIDs.Copy(Other.InactiveTIDs);
	this->Threads.Copy(Other.Threads);
}

pantheon::Process::Process(Process &&Other) noexcept
{
	this->CurState = Other.CurState;
	this->InactiveTIDs.Move(Other.InactiveTIDs);
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->Threads.Move(Other.Threads);
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
	this->InactiveTIDs = Other.InactiveTIDs;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->Threads = Other.Threads;
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
	this->InactiveTIDs.Move(Other.InactiveTIDs);
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

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	pantheon::Thread T(this);
	/* Attempt 128KB of stack space for now... */
	Optional<void*> StackSpace = BasicMalloc(128 * 1024);
	if (StackSpace.GetOkay() == FALSE)
	{
		return FALSE;
	}

	T.SetStackAddr((UINT64)StackSpace());

	pantheon::CpuContext &Regs = T.GetRegisters();
	Regs.SetPC((UINT64)(StartAddr));
	Regs.SetArg1((UINT64)ThreadData);
	this->InactiveTIDs.Add(T.ThreadID());
	this->Threads.Add(T);
	return TRUE;
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	return this->PID;
}


[[nodiscard]] 
UINT64 pantheon::Process::NumInactiveThreads() const
{
	return this->InactiveTIDs.Size();
}


static pantheon::Spinlock ActivateThreadLock;
pantheon::Thread* pantheon::Process::ActivateThread()
{
	ActivateThreadLock.Acquire();
	if (this->NumInactiveThreads() == 0)
	{
		ActivateThreadLock.Release();
		return nullptr;
	}

	UINT64 TID = this->InactiveTIDs[0];
	this->InactiveTIDs.Delete(0);
	
	for (UINT64 Index = 0; Index < this->Threads.Size(); ++Index)
	{
		if (this->Threads[Index].ThreadID() == TID)
		{
			this->Threads[Index].SetState(pantheon::THREAD_STATE_RUNNING);
			pantheon::Thread *Selected = &(this->Threads[Index]);
			ActivateThreadLock.Release();
			return Selected;
		}
	}
	
	ActivateThreadLock.Release();
	return nullptr;
}

void pantheon::Process::DeactivateThread(pantheon::Thread *T)
{
	ActivateThreadLock.Acquire();
	T->SetState(pantheon::THREAD_STATE_WAITING);
	this->InactiveTIDs.Add(T->ThreadID());
	ActivateThreadLock.Release();
}