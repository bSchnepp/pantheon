#include <kern.h>
#include <stddef.h>

#include <arch.hpp>
#include <sync.hpp>
#include <cpureg.hpp>
#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_cpu.hpp>
#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>
#include <System/Memory/kern_alloc.hpp>

#include <Common/Structures/kern_optional.hpp>
#include <Common/Structures/kern_skiplist.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

/**
 * @file Common/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and algorithms.
 * \~english @details Pantheon implements Con Kolivas' MuQSS scheduling algorithm, an efficient algorithm 
 *  which takes priorities into account to provide a semi-"realtime" scheduling system to ensure 
 *  fairness between processes. In contrast to BFS, lock contention is avoided by fairly distributing
 *  work between individual processors: in this case, threads are the fundamental schedulable unit,
 *  which is what is assigned virtual deadlines. Note that "realtime" is in quotes as to my knowledge, no definitive
 *  proof exists to guarantee that any given task will always meet it's virtual deadline, though all jobs should
 *  just by induction eventually all run anyway, even if they miss their deadlines.
 * @see http://ck.kolivas.org/patches/muqss/sched-MuQSS.txt
 * \~english @author Brian Schnepp
 */


static pantheon::Spinlock SchedLock("Global Scheduler Lock");
static pantheon::LinkedList<pantheon::Process> ProcessList;
static pantheon::SkipList<UINT64, pantheon::Thread*> ThreadList;


/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::LocalScheduler::LocalScheduler() : pantheon::Allocatable<LocalScheduler, 256>(), pantheon::Lockable("Scheduler")
{
	this->IdleThread = nullptr;
}

/* Provided by the arch/ subdir. These differ between hardware platforms. */
extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT64 RegOffset);
extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);


/**
 * \~english @brief Counts the number of threads under this scheduler which belong to a given PID.
 * \~english @invariant This scheduler is not locked
 * \~english @return The number of threads under this manager which belong to a given process 
 */
UINT64 pantheon::LocalScheduler::CountThreads(UINT64 PID)
{
	UINT64 Res = 0;
	pantheon::ScopedLock _L(this);
	for (const auto &Item : this->Threads)
	{
		Res += 1ULL * (Item.MyProc()->ProcessID() == PID);
	}
	return Res;
}

/**
 * \~english @brief Obtains a thread from the local runqueue which can be run, and removes it from the local runqueue.
 * \~english @invariant This scheduler is locked
 * \~english @return A thread which can be run, if it exists. Nullptr otherwise.
 */
pantheon::Thread *pantheon::LocalScheduler::AcquireThread()
{
	Optional<UINT64> LowestKey = this->LocalRunQueue.MinKey();
	if (!LowestKey.GetOkay())
	{
		return nullptr;
	}

	Optional<pantheon::Thread*> Lowest = this->LocalRunQueue.Get(LowestKey.GetValue());
	if (!Lowest.GetOkay())
	{
		return nullptr;
	}

	this->LocalRunQueue.Delete(LowestKey.GetValue());
	return Lowest.GetValue();

}

pantheon::Thread *pantheon::LocalScheduler::Idle()
{
	return this->IdleThread;
}

void pantheon::LocalScheduler::Setup()
{
	pantheon::ScopedGlobalSchedulerLock _L;

	pantheon::Thread *Idle = pantheon::Thread::Create();
	for (pantheon::Process &Proc : ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			Idle->Initialize(&Proc, nullptr, nullptr, pantheon::Thread::PRIORITY_VERYLOW, FALSE);
		}
	}

	pantheon::CPU::GetCoreInfo()->CurThread = Idle;
	this->IdleThread = Idle;	
}

static UINT64 CalculateDeadline(UINT64 Jiffies, UINT64 PrioRatio, UINT64 RRInterval)
{
	/* We don't necessarilly have a high resolution timer, so let's just use jiffies. */
	UINT64 Deadline = Jiffies + (PrioRatio * RRInterval);

	/* However, we have to ban -1, since we're using a SkipList. */
	if ((Deadline & ~0ULL) == ~0ULL)
	{
		Deadline = 0;
	}

	return Deadline;
}

/**
 * \~english @brief Inserts a thread for this local scheduler
 * \~english @invariant The thread to be inserted is locked before calling
 * \~english @invariant This scheduler is not locked
 * \~english @param Thr The thread object to insert.
 */
void pantheon::LocalScheduler::InsertThread(pantheon::Thread *Thr)
{
	if (Thr == nullptr)
	{
		return;
	}

	pantheon::ScopedLock _L(this);
	this->Threads.PushFront(Thr);
	if (Thr->MyState() == pantheon::Thread::STATE_WAITING)
	{
		UINT64 Jiffies = pantheon::CPU::GetJiffies();
		UINT64 Prio = pantheon::Thread::PRIORITY_MAX - Thr->MyPriority();
		this->LocalRunQueue.Insert(CalculateDeadline(Jiffies, Prio, 6), Thr);
	}
}

void pantheon::Scheduler::Init()
{
	static pantheon::Process IdleProc;
	ProcessList.PushFront(&IdleProc);
}

void pantheon::Scheduler::Lock()
{
	SchedLock.Acquire();
}

void pantheon::Scheduler::Unlock()
{
	SchedLock.Release();
}

/**
 * \~english @brief Creates a process, visible globally, from a name and address.
 * \~english @details A process is created, such that it can be run on any
 * available processor on the system. The given process name identifies the
 * process for debugging purposes and as a user-readable way to identify a
 * process, and the start address is a pointer to the first instruction the
 * initial threaq should execute when scheduled.
 * \~english @param[in] ProcStr A human-readable name for the process
 * \~english @param[in] StartAddr The initial value of the program counter
 * \~english @return New PID if the process was sucessfully created, 0 otherwise.
 * \~english @author Brian Schnepp
 */
UINT32 pantheon::Scheduler::CreateProcess(const pantheon::String &ProcStr, void *StartAddr)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	UINT32 NewID = pantheon::Scheduler::AcquireProcessID();

	pantheon::Process *Proc = pantheon::Process::Create();
	if (Proc == nullptr)
	{
		return 0;
	}

	pantheon::ScopedLock _SL(Proc);

	pantheon::ProcessCreateInfo CreateInfo{};
	CreateInfo.EntryPoint = reinterpret_cast<pantheon::vmm::VirtualAddress>(StartAddr);
	CreateInfo.Name = ProcStr;
	CreateInfo.ID = NewID;
	Proc->Initialize(CreateInfo);

	ProcessList.PushFront(Proc);

	return NewID;
}

pantheon::Thread *pantheon::Scheduler::CreateThread(UINT32 PID, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	pantheon::Process *MyProc = nullptr;
	for (pantheon::Process &Proc : ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			MyProc = &Proc;
			break;
		}
	}

	if(!MyProc)
	{
		/* Huh? You're asking for a PID that doesn't exist. */
		return nullptr;
	}

	pantheon::Thread *Thr = pantheon::Thread::Create();
	Thr->Initialize(MyProc, StartAddr, ThreadData, Priority, TRUE);
	ThreadList.Insert(Thr->ThreadID(), Thr);
	return Thr;
}


UINT64 pantheon::Scheduler::CountThreads(UINT64 PID)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	UINT64 Count = 0;
	UINT8 MyCPU = pantheon::CPU::GetProcessorNumber();
	UINT8 NoCPUs = pantheon::CPU::GetNoCPUs();
	for (UINT8 Index = 0; Index < NoCPUs; Index++)
	{
		UINT8 LocalIndex = (MyCPU + Index) % NoCPUs;
		Count += pantheon::CPU::GetLocalSched(LocalIndex)->CountThreads(PID);
	}
	return Count;
}

UINT32 pantheon::Scheduler::AcquireProcessID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 * 
	 * 0 should be reserved for the generic idle process.
	 */
	UINT32 RetVal = 0;
	static UINT32 ProcessID = 1;
	RetVal = ProcessID++;
	return RetVal;
}

UINT64 pantheon::Scheduler::AcquireThreadID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT64 RetVal = 0;
	static UINT64 ThreadID = 0;
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ThreadID++;
	
	/* Ban 0 and -1, since these are special values. */
	if (RetVal == 0 || RetVal == ~0ULL)
	{
		return pantheon::Scheduler::AcquireThreadID();
	}
	return RetVal;
}

static pantheon::Thread *GetNextThread()
{
	UINT8 MyCPU = pantheon::CPU::GetProcessorNumber();
	UINT8 NoCPUs = pantheon::CPU::GetNoCPUs();
	for (UINT8 Index = 0; Index < NoCPUs; Index++)
	{
		UINT8 LocalIndex = (MyCPU + Index) % NoCPUs;
		pantheon::LocalScheduler *Sched = pantheon::CPU::GetLocalSched(LocalIndex);
		if (Sched)
		{
			Sched->Lock();	/* This needs to be TryAcquire at some point. */
			pantheon::Thread *Thr = Sched->AcquireThread();
			Sched->Unlock();
			if (Thr != nullptr)
			{
				return Thr;
			}
		}
	}
	return nullptr;
}

struct SwapContext
{
	pantheon::CpuContext *Old;
	pantheon::CpuContext *New;
};

static SwapContext SwapThreads(pantheon::Thread *CurThread, pantheon::Thread *NextThread)
{
	/* If we have no next, give up and use the idle thread. */
	if (NextThread == nullptr)
	{
		NextThread = pantheon::CPU::GetMyLocalSched()->Idle();
	}

	pantheon::ScopedLock _NL(NextThread);

	/* Don't try to swap to ourselves. */
	if (NextThread == CurThread)
	{
		return {nullptr, nullptr};
	}

	pantheon::ScopedLock _OL(CurThread);

	if (NextThread->MyState() != pantheon::Thread::STATE_WAITING)
	{
		/* Huh? Why are we still here? */
		pantheon::StopErrorFmt("Thread %lx was on the runqueue, but wasn't waiting\n", NextThread->MyState());
	}

	/* Okay, we have to do a weird little dance. We need this to prevent
	 * trying to reschedule and trip on ourselves. This gets restored
	 * when we context switch back here.
	 */
	pantheon::CPU::GetCurThread()->BlockScheduling();

	/* Change contexts */
	pantheon::Process::Switch(NextThread->MyProc());
	pantheon::ipc::SetThreadLocalRegion(NextThread->GetThreadLocalAreaRegister());

	pantheon::Scheduler::SetThreadState(CurThread->ThreadID(), pantheon::Thread::STATE_WAITING);
	pantheon::Scheduler::SetThreadState(NextThread->ThreadID(), pantheon::Thread::STATE_RUNNING);

	pantheon::CpuContext *OldContext = CurThread->GetRegisters();
	pantheon::CpuContext *NewContext = NextThread->GetRegisters();

	pantheon::CPU::GetMyLocalSched()->InsertThread(CurThread);

	return {OldContext, NewContext};
}

/**
 * \~english @brief Changes the current thread of this core.
 * \~english @details Forcefully changes the current thread by iterating to
 * the next thread in the list of threads belonging to this core. The active
 * count for the current thread is set to zero, and the new thread is run until
 * either the core is forcefully rescheduled, or the timer indicates the current
 * thread should quit.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::Reschedule()
{
	/* Don't reschedule if interrupts are off */
	if (pantheon::CPU::ICOUNT())
	{
		return;
	}

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Thread *NextThread = GetNextThread();

	SwapContext Con = SwapThreads(CurThread, NextThread);

	if (Con.New && Con.Old)
	{
		pantheon::Sync::DSBISH();
		pantheon::Sync::ISB();
		cpu_switch(Con.Old, Con.New, CpuIRegOffset);
		pantheon::CPU::GetCurThread()->EnableScheduling();
	}
}

void pantheon::Scheduler::AttemptReschedule()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();

	if (CurThread)
	{
		CurThread->Lock();
		CurThread->CountTick();
		UINT64 RemainingTicks = CurThread->TicksLeft();

		if (RemainingTicks > 0 || CurThread->Preempted())
		{
			CurThread->Unlock();
			return;
		}
		
		CurThread->SetTicks(0);
		CurThread->Unlock();


		if (pantheon::CPU::ICOUNT() != 0)
		{
			StopError("Interrupts not allowed for reschedule");
		}

		pantheon::Scheduler::Reschedule();
	}
}

extern "C" VOID FinishThread()
{
	pantheon::CPU::GetCurThread()->EnableScheduling();
}

BOOL pantheon::Scheduler::MapPages(UINT32 PID, const pantheon::vmm::VirtualAddress *VAddresses, const pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages)
{
	/* TODO: Move into a more sensible namespace like pantheon::mm or something */
	pantheon::ScopedGlobalSchedulerLock _L;
	for (pantheon::Process &Item : ProcessList)
	{
		if (Item.ProcessID() == PID)
		{
			pantheon::ScopedLock _LL(&Item);
			for (UINT64 Index = 0; Index < NumPages; ++Index)
			{
				Item.MapAddress(VAddresses[Index], PAddresses[Index], PageAttributes);
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL pantheon::Scheduler::SetState(UINT32 PID, pantheon::Process::State State)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	for (pantheon::Process &Item : ProcessList)
	{
		if (Item.ProcessID() == PID)
		{
			pantheon::ScopedLock _LL(&Item);
			Item.SetState(State);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL pantheon::Scheduler::SetThreadState(UINT64 TID, pantheon::Thread::State State)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	if (!ThreadList.Contains(TID))
	{
		return FALSE;
	}

	pantheon::Thread *Thr = ThreadList[TID];
	pantheon::ScopedLock _LL(Thr);

	Thr->SetState(State);
	return TRUE;
}