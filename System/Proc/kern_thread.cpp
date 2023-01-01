#include <sync.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include "kern_cpu.hpp"
#include "kern_proc.hpp"
#include "kern_sched.hpp"
#include "kern_thread.hpp"

#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>

#include <System/Memory/kern_alloc.hpp>

/**
 * \~english @brief Prepares a thread ready to have contents moved to it.
 * \~english @author Brian Schnepp
 */
pantheon::Thread::Thread() : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = nullptr;
	this->PreemptCount = 1;
	this->CurPriority = pantheon::Thread::PRIORITY_NORMAL;
	this->CurState = pantheon::Thread::STATE_TERMINATED;
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
	: pantheon::Thread::Thread(OwningProcess, pantheon::Thread::PRIORITY_NORMAL)
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
pantheon::Thread::Thread(Process *OwningProcess, Priority Pri) : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = OwningProcess;
	this->CurPriority = Pri;
	this->KernelStackSpace = nullptr;
	this->UserStackSpace = nullptr;

	this->PreemptCount = 1;
	this->RemainingTicks = 0;

	this->Registers.Wipe();
	this->CurState = pantheon::Thread::STATE_INIT;

	this->TID = 0;
	this->Unlock();
}

pantheon::Thread::Thread(const pantheon::Thread &Other) : pantheon::Thread()
{
	this->Lock();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->CurPriority = Other.CurPriority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->CurState = Other.CurState;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->LocalRegion = Other.LocalRegion;
	this->Unlock();
}

pantheon::Thread::Thread(pantheon::Thread &&Other) noexcept : pantheon::Lockable("Thread")
{
	this->Lock();
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->CurPriority = Other.CurPriority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->CurState = Other.CurState;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	this->LocalRegion = Other.LocalRegion;
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
	OBJECT_SELF_ASSERT();
	return this->ParentProcess;
}

/**
 * \~english @brief Gets the state, such as running or blocked, of this thread.
 * \~english @author Brian Schnepp
 * \~english @return The current status of this thread
 */
[[nodiscard]]
pantheon::Thread::State pantheon::Thread::MyState() const
{
	OBJECT_SELF_ASSERT();
	UINT64 State = this->CurState;
	if (State > pantheon::Thread::STATE_MAX)
	{
		StopErrorFmt("Invalid thread state: got 0x%lx\n", this->CurState);
	}
	return this->CurState;
}

/**
 * \~english @brief Gets the priority of the thread, used for time allocation.
 * \~english @author Brian Schnepp
 * \~english @return The current priority of this thread
 */
[[nodiscard]]
pantheon::Thread::Priority pantheon::Thread::MyPriority() const
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("MyPriority without lock");
	}
	return this->CurPriority;
}

/**
 * \~english @brief Checks if this thread is valid to interrupt at this moment
 * \~english @author Brian Schnepp
 * \~english @return 0 if not doing system work currently, 1 otherwise.
 */
[[nodiscard]]
BOOL pantheon::Thread::Preempted() const
{
	OBJECT_SELF_ASSERT();
	return this->PreemptCount != 0;
}

/**
 * \~english @brief Counts down the number of timer interrupts for this thread
 * \~english @author Brian Schnepp
 */
INT64 pantheon::Thread::CountTick()
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedLock _L(this);
	if (this->RemainingTicks.Load() <= 0)
	{
		this->SetTicks(0);
		return 0;
	}
	return this->RemainingTicks.Sub(1);
}

[[nodiscard]]
UINT64 pantheon::Thread::ThreadID() const
{
	OBJECT_SELF_ASSERT();
	return this->TID;
}

/**
 * \~english @brief Sets the state, such as running, to the current process.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetState(Thread::State St)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("SetState without lock", this);
	}	
	this->CurState = St;
}

VOID pantheon::Thread::SetTicks(INT64 TickCount)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("SetTicks without lock", this);
	}
	this->RemainingTicks = (TickCount < 0) ? 0 : TickCount;
}

/**
 * \~english @brief Changes the priority of the current thread
 * \~english @details Informs the kernel that this thread should be scheduled
 * less often as compared to others. This generally happens for something such
 * as a network services daemon, which does not need to run too often.
 * \~english @author Brian Schnepp
 */
VOID pantheon::Thread::SetPriority(Thread::Priority Pri)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("SetPriority without lock");
	}

	if (Pri <= this->CurPriority)
	{
		this->CurPriority = Pri;
	}
}

/**
 * \~english @brief Obtains a reference to the registers of the thread.
 * \~english @author Brian Schnepp
 */
pantheon::CpuContext *pantheon::Thread::GetRegisters()
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("GetRegisters without lock");
	}

	/* TODO: Copy the actual registers to the internal representation! */
	return &this->Registers;
}

pantheon::Thread &pantheon::Thread::operator=(const pantheon::Thread &Other)
{
	OBJECT_SELF_ASSERT();
	if (this == &Other)
	{
		return *this;
	}

	Lockable::operator=(Other);
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->CurPriority = Other.CurPriority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->CurState = Other.CurState;
	this->TID = Other.TID;

	/* Is this right? */
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	return *this;
}

pantheon::Thread &pantheon::Thread::operator=(pantheon::Thread &&Other) noexcept
{
	OBJECT_SELF_ASSERT();
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->ParentProcess = Other.ParentProcess;
	this->PreemptCount = Other.PreemptCount;
	this->CurPriority = Other.CurPriority;
	this->Registers = Other.Registers;
	this->RemainingTicks = Other.RemainingTicks;
	this->CurState = Other.CurState;
	this->TID = Other.TID;
	this->KernelStackSpace = Other.KernelStackSpace;
	this->UserStackSpace = Other.UserStackSpace;
	return *this;
}

void pantheon::Thread::BlockScheduling()
{
	OBJECT_SELF_ASSERT();
	this->PreemptCount.Add(1LL);
}

void pantheon::Thread::EnableScheduling()
{
	OBJECT_SELF_ASSERT();
	this->PreemptCount.Sub(1LL);
}

extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);

static void true_drop_process(void *StartAddress, pantheon::vmm::VirtualAddress StackAddr)
{
	/* This gets usermode for aarch64, needs to be abstracted! */
	drop_usermode((UINT64)StartAddress, 0x00, StackAddr);
}

void pantheon::Thread::Initialize(UINT64 TID, pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority, BOOL UserMode)
{
	static constexpr UINT64 InitialThreadStackSize = pantheon::vmm::SmallestPageSize * pantheon::Thread::InitialNumStackPages;
	this->TID = TID;
	this->CurState = pantheon::Thread::STATE_DEAD;

	this->Lock();

	Optional<void*> StackSpace = BasicMalloc(InitialThreadStackSize);
	if (StackSpace.GetOkay())
	{
		UINT64 IStackSpace = (UINT64)StackSpace();
		#ifdef POISON_MEMORY
		SetBufferBytes((UINT8*)IStackSpace, 0xAF, InitialThreadStackSize);
		#endif
		IStackSpace += InitialThreadStackSize;
		this->ParentProcess = Proc;

		UINT64 IStartAddr = (UINT64)true_drop_process;
		UINT64 IThreadData = (UINT64)ThreadData;

		pantheon::CpuContext *Regs = this->GetRegisters();
		Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
		if (UserMode)
		{
			Regs->SetInitUserContext(pantheon::Process::StackAddr, (UINT64)StartAddr);
		}
		this->SetState(pantheon::Thread::STATE_WAITING);
		this->SetPriority(Priority);
	}

	/* Don't create a TLS for kernel threads */
	if (UserMode)
	{
		this->LocalRegion = pantheon::Process::ThreadLocalBase + (this->ThreadID() * 0x1000);

		/* Map in the TLR. */
		pantheon::vmm::PageTableEntry Entry;
		Entry.SetBlock(TRUE);
		Entry.SetMapped(TRUE);
		Entry.SetUserNoExecute(TRUE);
		Entry.SetKernelNoExecute(TRUE);

		/* Yuck. But we need to make page permissions less bad...*/
		UINT64 PagePermission = 0x1 << 6;
		Entry.SetPagePermissions(PagePermission);

		Entry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
		Entry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
		Entry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

		pantheon::ScopedLock _L(this->MyProc());
		this->MyProc()->MapAddress(this->LocalRegion, pantheon::PageAllocator::Alloc(), Entry);
	}
	this->Unlock();	
}

pantheon::Thread::ThreadLocalRegion *pantheon::Thread::GetThreadLocalArea() 
{
	/* Assume Proc is locked */
	pantheon::Process *Proc = this->ParentProcess;
	pantheon::vmm::PhysicalAddress PhyAddr = pantheon::vmm::VirtualToPhysicalAddress(Proc->GetPageTable(), this->LocalRegion);
	pantheon::vmm::VirtualAddress VirtAddr = pantheon::vmm::PhysicalToVirtualAddress(PhyAddr);
	return reinterpret_cast<ThreadLocalRegion*>(VirtAddr);
}

/**
 * @brief Switches thread context to another thread
 */
void pantheon::Thread::Switch(pantheon::Thread *NextThread)
{
	if (NextThread == nullptr)
	{
		return;
	}

	pantheon::Thread *CurThread = pantheon::CPU::GetCoreInfo()->CurThread;
	pantheon::CPU::GetCurThread()->SetState(pantheon::Thread::STATE_WAITING);
	NextThread->SetState(Thread::STATE_RUNNING);
	NextThread->SetTicks(Thread::RR_INTERVAL);

	pantheon::ipc::SetThreadLocalRegion(NextThread->LocalRegion);
	
	pantheon::CPU::GetCoreInfo()->CurThread = NextThread;
	pantheon::CPU::GetMyLocalSched()->InsertThread(CurThread);
}