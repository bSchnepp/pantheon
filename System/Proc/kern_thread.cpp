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
pantheon::Thread::Thread()
{
	this->ThreadLock = pantheon::Spinlock("thread_lock");
	this->ThreadLock.Acquire();
	this->ParentProcess = nullptr;
	this->PreemptCount = 0;
	this->Priority = pantheon::THREAD_PRIORITY_NORMAL;
	this->State = pantheon::THREAD_STATE_TERMINATED;
	this->RemainingTicks = 0;
	this->Registers.Wipe();
	this->KernelStackSpace = nullptr;
	this->UserStackSpace = nullptr;
	this->TID = 0;
	this->VisitFlag = FALSE;

	/* TODO: Create new page tables, instead of reusing old stuff. */
	this->TTBR0 = (void*)pantheon::CPUReg::R_TTBR0_EL1();
	this->ThreadLock.Release();
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
pantheon::Thread::Thread(Process *OwningProcess, ThreadPriority Priority)
{
	this->ThreadLock = pantheon::Spinlock("thread_lock");
	this->ThreadLock.Acquire();
	this->ParentProcess = OwningProcess;
	this->Priority = Priority;
	this->KernelStackSpace = nullptr;
	this->UserStackSpace = nullptr;

	this->PreemptCount = 0;
	this->RemainingTicks = 0;

	this->Registers.Wipe();
	this->State = pantheon::THREAD_STATE_INIT;

	this->TID = AcquireThreadID();
	this->VisitFlag = FALSE;

	/* TODO: Create new page tables, instead of reusing old stuff. */
	this->TTBR0 = (void*)pantheon::CPUReg::R_TTBR0_EL1();

	/* 45 for NORMAL, 30 for LOW, 15 for VERYLOW, etc. */
	this->RefreshTicks();
	this->ThreadLock.Release();
}

pantheon::Thread::Thread(const pantheon::Thread &Other)
{
	this->ThreadLock = pantheon::Spinlock("thread_lock");
	this->ThreadLock.Acquire();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->VisitFlag = Other.VisitFlag;
	this->TTBR0 = (void*)Other.TTBR0;	
	this->ThreadLock.Release();
}

pantheon::Thread::Thread(pantheon::Thread &&Other) noexcept
{
	this->ThreadLock = pantheon::Spinlock("thread_lock");
	this->ThreadLock.Acquire();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->Priority = Other.Priority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->State = Other.State;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->VisitFlag = Other.VisitFlag;
	this->TTBR0 = (void*)Other.TTBR0;
	this->ThreadLock.Release();
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
	return this->Priority;
}

/**
 * \~english @brief Gets the number of times this process has been pre-empted.
 * \~english @author Brian Schnepp
 * \~english @return The number of times the kernel has interrupted this thread
 * to resume execution of some other work.
 */
[[nodiscard]]
UINT64 pantheon::Thread::Preempts() const
{
	return this->PreemptCount;
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

[[nodiscard]] 
void *pantheon::Thread::GetTTBR0() const
{
	return this->TTBR0;
}

/**
 * \~english @brief Forcefully adds execution time to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::AddTicks(UINT64 TickCount)
{
	this->RemainingTicks += TickCount;
}

VOID pantheon::Thread::RefreshTicks()
{
	this->RemainingTicks = (this->Priority + 1) * 3;
}

/**
 * \~english @brief Sets the state, such as running, to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetState(ThreadState State)
{
	this->State = State;
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
	/* TODO: Copy the actual registers to the internal representation! */
	return &this->Registers;
}

pantheon::Thread &pantheon::Thread::operator=(const pantheon::Thread &Other)
{
	if (this == &Other)
	{
		return *this;
	}
	this->ThreadLock = pantheon::Spinlock("thread_lock");
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
	this->ThreadLock = pantheon::Spinlock("thread_lock");
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
	this->Registers.SetSP(Addr);
	this->KernelStackSpace = reinterpret_cast<void*>((CHAR*)Addr);
}

void pantheon::Thread::SetUserStackAddr(UINT64 Addr)
{
	this->Registers.SetSP(Addr);
	this->UserStackSpace = reinterpret_cast<void*>((CHAR*)Addr);
}

void pantheon::Thread::FlipVisitFlag()
{
	this->VisitFlag = !this->VisitFlag;
}

[[nodiscard]]
BOOL pantheon::Thread::GetVisitFlag() const
{
	return this->VisitFlag;
}

VOID pantheon::Thread::Lock()
{
	this->ThreadLock.Acquire();
}

VOID pantheon::Thread::Unlock()
{
	this->ThreadLock.Release();
}

void pantheon::Thread::SetProc(pantheon::Process *Proc)
{
	this->ParentProcess = Proc;
}