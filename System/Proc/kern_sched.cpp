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
 * @file System/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and algorithms.
 * \~english @details Pantheon implements Con Kolivas' MuQSS scheduling algorithm, an efficient algorithm 
 *  which takes priorities into account to provide a fair scheduling system to ensure 
 *  fairness between processes. In contrast to BFS, lock contention is avoided by fairly distributing
 *  work between individual processors: in this case, threads are the fundamental schedulable unit,
 *  which is what is assigned virtual deadlines.
 *  Some details are either simplified or ignored: this implementation ignores nice levels, and ignores cache locality, SMT thread groups,
 *  and does not use a high precision timer (relying on jiffies wherever the corresponding Linux patch uses niffies), and lacks any runtime tunable parameters.
 * @see http://ck.kolivas.org/patches/muqss/sched-MuQSS.txt
 * \~english @author Brian Schnepp
 */


static pantheon::Spinlock SchedLock("Global Scheduler Lock");
static pantheon::SkipList<UINT32, pantheon::Process*> ProcessList;
static pantheon::SkipList<UINT64, pantheon::Thread*> ThreadList;

pantheon::LocalScheduler::LocalScheduler() : pantheon::Lockable("Scheduler")
{
	/* Because of the ordering required for init_array, the actual
	 * setup must be done with the Setup() function, not the constructor.
	 */
	this->IdleThread = nullptr;
	this->Ready = FALSE;
}

/** 
 * @private
 * @brief Provided by the arch/ subdir. Swaps the current context
 */
extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT64 RegOffset);

/** 
 * @private
 * @brief Provided by the arch/ subdir. Drops a new thread to have a different PSTATE
 */
extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);

UINT64 pantheon::LocalScheduler::CountThreads(UINT64 PID)
{
	if (!this->Ready)
	{
		this->Setup();
	}

	UINT64 Res = 0;
	pantheon::ScopedLock _L(this);
	for (const auto &Item : this->Threads)
	{
		Res += 1ULL * (Item.MyProc()->ProcessID() == PID);
	}
	return Res;
}

pantheon::Thread *pantheon::LocalScheduler::AcquireThread()
{
	if (!this->Ready)
	{
		this->Setup();
	}

	if (!this->IsLocked())
	{
		pantheon::StopErrorFmt("Trying to AcquireThread without lock\n");
	}

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

	UINT64 Key = LowestKey.GetValue();
	this->LocalRunQueue.Delete(Key);

	pantheon::Thread *Value = Lowest.GetValue();
	Value->Lock();
	return Value;

}

/**
 * @private
 * @brief Obtains the idle thread
 */
pantheon::Thread *pantheon::LocalScheduler::Idle()
{
	if (!this->Ready)
	{
		this->Setup();
	}

	return this->IdleThread;
}

[[nodiscard]] 
UINT64 pantheon::LocalScheduler::GetLowestDeadline() const
{
	Optional<UINT64> Val = this->LocalRunQueue.MinKey();
	return Val.GetOkay() ? Val.GetValue() : -1;
}

static UINT32 AcquireProcessID()
{
	UINT32 RetVal = 0;
	static UINT32 ProcessID = 1;

	if (!SchedLock.IsLocked())
	{
		pantheon::StopErrorFmt("Acquire thread without locking Global Scheduler\n");
	}

	RetVal = ProcessID++;
	while (ProcessList.Contains(RetVal) || RetVal == 0)
	{
		RetVal = ProcessID++;
	}

	return RetVal;
}

static UINT64 AcquireThreadID()
{
	UINT64 RetVal = 0;
	static UINT64 ThreadID = 1;

	if (!SchedLock.IsLocked())
	{
		pantheon::StopErrorFmt("Acquire thread without locking Global Scheduler\n");
	}

	RetVal = ThreadID++;
	/* Ban 0 and -1, since these are special values. */
	while (RetVal == 0 || RetVal == ~0ULL || ThreadList.Contains(RetVal))
	{
		RetVal = ThreadID++;
	}
	return RetVal;
}

/**
 * @private
 * @brief Sets up a local scheduler 
 */
void pantheon::LocalScheduler::Setup()
{
	pantheon::ScopedGlobalSchedulerLock _L;

	pantheon::Thread *Idle = pantheon::Thread::Create();
	Optional<pantheon::Process*> MProc = ProcessList.Get(0);
	if (!MProc.GetOkay())
	{
		pantheon::StopErrorFmt("No PID 0!\n");
	}

	Idle->Initialize(AcquireThreadID(), MProc.GetValue(), nullptr, nullptr, pantheon::Thread::PRIORITY_VERYLOW, FALSE);
	Idle->Lock();
	Idle->SetState(pantheon::Thread::STATE_RUNNING);

	pantheon::CPU::GetCoreInfo()->CurThread = Idle;
	this->IdleThread = Idle;
	this->Ready = TRUE;

	pantheon::CPU::GetMyLocalSched()->InsertThread(Idle);	
	Idle->Unlock();
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

void pantheon::LocalScheduler::InsertThread(pantheon::Thread *Thr)
{
	if (Thr == nullptr)
	{
		return;
	}

	if (!Thr->IsLocked())
	{
		pantheon::StopErrorFmt("Attempting to insert thread which wasn't locked into runqueue\n");
	}

	pantheon::ScopedLock _L(this);
	if (Thr->MyState() == pantheon::Thread::STATE_WAITING)
	{
		UINT64 Jiffies = pantheon::CPU::GetJiffies();
		UINT64 Prio = pantheon::Thread::PRIORITY_MAX - Thr->MyPriority();

		UINT64 Deadline = CalculateDeadline(Jiffies, Prio, pantheon::Thread::RR_INTERVAL);
		while (this->LocalRunQueue.Contains(Deadline))
		{
			Deadline++;
		}
		this->LocalRunQueue.Insert(Deadline, Thr);
	}
}


/**
 * @private
 * @brief Sets up a global scheduler 
 */
void pantheon::Scheduler::Init()
{
	/* This is probably unnecessary, but let's be safe... */
	ScopedGlobalSchedulerLock _L;

	static pantheon::Process IdleProc;
	ProcessList.Insert(0, &IdleProc);
}

void pantheon::Scheduler::Lock()
{
	SchedLock.Acquire();
}

void pantheon::Scheduler::Unlock()
{
	SchedLock.Release();
}

UINT32 pantheon::Scheduler::CreateProcess(const pantheon::String &ProcStr, void *StartAddr)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	UINT32 NewID = AcquireProcessID();

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

	ProcessList.Insert(NewID, Proc);

	return NewID;
}

UINT64 pantheon::Scheduler::CreateThread(UINT32 PID, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	if(!ProcessList.Contains(PID))
	{
		/* Huh? You're asking for a PID that doesn't exist. */
		return 0;
	}

	pantheon::Process *MyProc = ProcessList.Get(PID).GetValue();
	pantheon::Thread *Thr = pantheon::Thread::Create();
	Thr->Initialize(AcquireThreadID(), MyProc, StartAddr, ThreadData, Priority, TRUE);
	ThreadList.Insert(Thr->ThreadID(), Thr);
	
	pantheon::ScopedLock _STL(Thr);
	pantheon::CPU::GetMyLocalSched()->InsertThread(Thr);
	return Thr->ThreadID();
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

static pantheon::Thread *GetNextThread()
{
	UINT8 MyCPU = pantheon::CPU::GetProcessorNumber();
	UINT8 NoCPUs = pantheon::CPU::GetNoCPUs();

	UINT64 LowestDeadline = -1;
	UINT8 LowestScheduler = MyCPU;

	for (UINT8 Index = 0; Index < NoCPUs; Index++)
	{
		UINT8 LocalIndex = (MyCPU + Index) % NoCPUs;
		pantheon::LocalScheduler *Sched = pantheon::CPU::GetLocalSched(LocalIndex);
		if (Sched)
		{
			UINT64 Deadline = Sched->GetLowestDeadline();
			if (Deadline < LowestDeadline)
			{
				LowestScheduler = LocalIndex;
				LowestDeadline = Deadline;
			}
		}
	}

	for (UINT8 Index = 0; Index < NoCPUs; Index++)
	{
		UINT8 LocalIndex = (LowestScheduler + Index) % NoCPUs;
		pantheon::LocalScheduler *Sched = pantheon::CPU::GetLocalSched(LocalIndex);
		if (Sched && Sched->TryLock())
		{
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

void pantheon::Scheduler::Reschedule()
{
	/* Interrupts must be enabled before we can do anything. */
	if (pantheon::CPU::ICOUNT())
	{
		return;
	}

	pantheon::CPU::GetCurThread()->Lock();
	pantheon::CPU::GetCurThread()->BlockScheduling();
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();

	pantheon::Thread *NextThread = GetNextThread();
	if (NextThread == nullptr)
	{
		NextThread = pantheon::CPU::GetMyLocalSched()->Idle();
		if (NextThread->IsLocked())
		{
			CurThread->Unlock();
			pantheon::CPU::GetCurThread()->EnableScheduling();
			return;
		}
		NextThread->Lock();
	}

	if (NextThread == CurThread)
	{
		CurThread->Unlock();
		pantheon::CPU::GetCurThread()->EnableScheduling();
		return;
	}

	if (NextThread->MyState() == pantheon::Thread::STATE_RUNNING 
		|| CurThread->MyState() != pantheon::Thread::STATE_RUNNING)
	{
		NextThread->Unlock();
		CurThread->Unlock();
		pantheon::CPU::GetCurThread()->EnableScheduling();
		return;
	}

	pantheon::Thread::Switch(CurThread, NextThread);
	pantheon::Process::Switch(NextThread->MyProc());

	pantheon::CpuContext *OldContext = CurThread->GetRegisters();
	pantheon::CpuContext *NewContext = NextThread->GetRegisters();

	pantheon::CPU::GetMyLocalSched()->InsertThread(CurThread);
	NextThread->Unlock();
	CurThread->Unlock();

	cpu_switch(OldContext, NewContext, CpuIRegOffset);
	pantheon::CPU::GetCurThread()->EnableScheduling();
}

void pantheon::Scheduler::AttemptReschedule()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	INT64 RemainingTicks = CurThread->CountTick();

	if (RemainingTicks <= 0 && !CurThread->Preempted())
	{
		pantheon::Scheduler::Reschedule();
	}
}

/** 
 * @private
 * @brief Finish routine for jumping to userspace
 */
extern "C" VOID FinishThread()
{
	pantheon::CPU::GetCurThread()->EnableScheduling();
}

BOOL pantheon::Scheduler::MapPages(UINT32 PID, const pantheon::vmm::VirtualAddress *VAddresses, const pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages)
{
	/* TODO: Move into a more sensible namespace like pantheon::mm or something */
	pantheon::ScopedGlobalSchedulerLock _L;

	if (!ProcessList.Contains(PID))
	{
		return FALSE;
	}

	pantheon::Process *Proc = ProcessList.Get(PID).GetValue();
	pantheon::ScopedLock _LL(Proc);
	for (UINT64 Index = 0; Index < NumPages; ++Index)
	{
		Proc->MapAddress(VAddresses[Index], PAddresses[Index], PageAttributes);
	}
	return TRUE;
}

BOOL pantheon::Scheduler::SetState(UINT32 PID, pantheon::Process::State State)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	if (!ProcessList.Contains(PID))
	{
		return FALSE;
	}

	pantheon::Process *Proc = ProcessList.Get(PID).GetValue();
	pantheon::ScopedLock _LL(Proc);
	Proc->SetState(State);
	return TRUE;
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