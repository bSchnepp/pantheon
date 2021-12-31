#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_cpu.hpp"
#include "kern_proc.hpp"
#include "kern_sched.hpp"
#include "kern_thread.hpp"

#include <System/PhyMemory/kern_alloc.hpp>

/**
 * \~english @brief Prepares a thread ready to have contents moved to it.
 * \~english @author Brian Schnepp
 */
pantheon::Thread::Thread() : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = nullptr;
	this->PreemptCount = 1;
	this->Priority = pantheon::THREAD_PRIORITY_NORMAL;
	this->State = pantheon::THREAD_STATE_TERMINATED;
	this->RemainingTicks = 0;
	this->Registers.Wipe();
	this->KernelStackSpace = nullptr;
	this->UserStackSpace = nullptr;
	this->TID = 0;
	this->Unlock();
}

/**
 * \~english @brief Initializes a new thread belonging to a process.
 * \~english @author Brian Schnepp
 * \~english @param OwningProcess A pointer to the owning process for which to
 * register the current thread.
 */
pantheon::Thread::Thread(Process *OwningProcess) 
	: pantheon::Thread::Thread(OwningProcess, pantheon::THREAD_PRIORITY_NORMAL)
{
}

/**
 * \~english @brief Initializes a new thread belonging to a process.
 * \~english @author Brian Schnepp
 * \~english @param OwningProcess A pointer to the owning process for which to
 * register the current thread.
 * \~english @param Priority The priority of the current thread. The thread
 * priority given will never be greater than the supplied priority. 
 */
pantheon::Thread::Thread(Process *OwningProcess, ThreadPriority Priority) : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = OwningProcess;
	this->Priority = Priority;
	this->KernelStackSpace = nullptr;
	this->UserStackSpace = nullptr;

	this->PreemptCount = 1;
	this->RemainingTicks = 0;

	this->Registers.Wipe();
	this->State = pantheon::THREAD_STATE_INIT;

	this->TID = AcquireThreadID();

	/* 45 for NORMAL, 30 for LOW, 15 for VERYLOW, etc. */
	this->RefreshTicks();
	this->Unlock();
}

pantheon::Thread::Thread(const pantheon::Thread &Other) : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->Unlock();
}

pantheon::Thread::Thread(pantheon::Thread &&Other) noexcept : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->Unlock();
}

pantheon::Thread::~Thread()
{
	if (this->KernelStackSpace)
	{
		BasicFree(this->KernelStackSpace);
	}

	if (this->UserStackSpace)
	{
		pantheon::PageAllocator::Free((UINT64)this->UserStackSpace);
	}
}

/**
 * \~english @brief Obtains a handle to the process that owns this thread
 * \~english @author Brian Schnepp
 * \~english @return A pointer to the owning process of this thread.
 */
[[nodiscard]]
pantheon::Process *pantheon::Thread::MyProc() const
{
	return this->ParentProcess;
}

/**
 * \~english @brief Gets the state, such as running or blocked, of this thread.
 * \~english @author Brian Schnepp
 * \~english @return The current status of this thread
 */
pantheon::ThreadState pantheon::Thread::MyState()
{
	if (this->IsLocked() == FALSE)
	{
		StopError("MyState without lock");
	}
	return this->State;
}

/**
 * \~english @brief Gets the priority of the thread, used for time allocation.
 * \~english @author Brian Schnepp
 * \~english @return The current priority of this thread
 */
[[nodiscard]]
pantheon::ThreadPriority pantheon::Thread::MyPriority()
{
	if (this->IsLocked() == FALSE)
	{
		StopError("MyPriority without lock");
	}
	return this->Priority;
}

/**
 * \~english @brief Checks if this thread is valid to interrupt at this moment
 * \~english @author Brian Schnepp
 * \~english @return 0 if not doing system work currently, 1 otherwise.
 */
[[nodiscard]]
BOOL pantheon::Thread::Preempted() const
{
	return this->PreemptCount != 0;
}

/**
 * \~english @brief Gets the number of timer interrupts remaining for the thread
 * \~english @author Brian Schnepp
 * \~english @return The number of times the processor core will need to be
 * interrupted before the thread is preempted and some other work resumes on
 * the current processor core.
 */
UINT64 pantheon::Thread::TicksLeft() const
{
	return this->RemainingTicks;
}

/**
 * \~english @brief Counts down the number of timer interrupts for this thread
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::CountTick()
{
	if (this->IsLocked() == FALSE)
	{
		StopError("CountTicks without lock");
	}

	if (this->RemainingTicks)
	{
		this->RemainingTicks--;
	}
}

[[nodiscard]]
UINT64 pantheon::Thread::ThreadID() const
{
	return this->TID;
}

/**
 * \~english @brief Forcefully adds execution time to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::AddTicks(UINT64 TickCount)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("AddTicks without lock");
	}
	this->RemainingTicks += TickCount;
}

VOID pantheon::Thread::RefreshTicks()
{
	if (this->IsLocked() == FALSE)
	{
		StopError("RefreshTicks without lock");
	}
	this->RemainingTicks = static_cast<UINT64>((this->Priority + 1)) * 3;
}

/**
 * \~english @brief Sets the state, such as running, to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetState(ThreadState State)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetState without lock");
	}	
	this->State = State;
}

VOID pantheon::Thread::SetTicks(UINT64 TickCount)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetTicks without lock");
	}
	this->RemainingTicks = TickCount;
}

/**
 * \~english @brief Changes the priority of the current thread
 * \~english @details Informs the kernel that this thread should be scheduled
 * less often as compared to others. This generally happens for something such
 * as a network services daemon, which does not need to run too often.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetPriority(ThreadPriority Priority)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetPriority without lock");
	}

	if (Priority <= this->Priority)
	{
		this->Priority = Priority;
	}
}

/**
 * \~english @brief Obtains a reference to the registers of the thread.
 * \~english @author Brian Schnepp
 */
pantheon::CpuContext *pantheon::Thread::GetRegisters()
{
	if (this->IsLocked() == FALSE)
	{
		StopError("GetRegisters without lock");
	}

	/* TODO: Copy the actual registers to the internal representation! */
	return &this->Registers;
}

pantheon::Thread &pantheon::Thread::operator=(const pantheon::Thread &Other)
{
	if (this == &Other)
	{
		return *this;
	}

	Lockable::operator=(Other);
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;

	/* Is this right? */
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	return *this;
}

pantheon::Thread &pantheon::Thread::operator=(pantheon::Thread &&Other) noexcept
{
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	return *this;
}

void pantheon::Thread::SetKernelStackAddr(UINT64 Addr)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetKernelStackAddr without lock");
	}

	this->Registers.SetSP(Addr);
	this->KernelStackSpace = reinterpret_cast<void*>((CHAR*)Addr);
}

void pantheon::Thread::SetUserStackAddr(UINT64 Addr)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetUserStackAddr without lock");
	}

	this->Registers.SetSP(Addr);
	this->UserStackSpace = reinterpret_cast<void*>((CHAR*)Addr);
}

void pantheon::Thread::SetProc(pantheon::Process *Proc)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("SetProc without lock");
	}	
	this->ParentProcess = Proc;
}

void pantheon::Thread::BlockScheduling()
{
	this->Lock();
	pantheon::Sync::DSBISH();
	this->PreemptCount++;
	pantheon::Sync::DSBISH();
	this->Unlock();
}

void pantheon::Thread::EnableScheduling()
{
	this->Lock();
	pantheon::Sync::DSBISH();
	this->PreemptCount--;
	pantheon::Sync::DSBISH();
	this->Unlock();
}