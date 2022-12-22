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

#include <Common/Structures/kern_skiplist.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

#ifndef ONLY_TESTING
extern "C" CHAR *USER_BEGIN;
extern "C" CHAR *USER_END;
#else
static UINT64 USER_BEGIN = 0;
static UINT64 USER_END = 0;
#endif

/**
 * @file Common/Proc/kern_sched.cpp
 * \~english @brief Definitions for basic kernel scheduling data structures and
 * algorithms. The pantheon kernel implements a basic round-robin style scheduling
 * algorithm based on tick counts and a list of threads.
 * \~english @author Brian Schnepp
 */

/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::Scheduler::Scheduler()
{
	this->IdleThread = pantheon::GlobalScheduler::CreateProcessorIdleThread();
	this->CurThread = this->IdleThread;	
}

pantheon::Scheduler::~Scheduler()
{

}

extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT64 RegOffset);

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
	OBJECT_SELF_ASSERT();
	/* Interrupts must be enabled before we can do anything. */
	if (pantheon::CPU::ICOUNT())
	{
		return;
	}

	GlobalScheduler::Lock();
	pantheon::Thread *Old = this->CurThread;
	pantheon::Thread *New = GlobalScheduler::PopFromReadyList();

	/* If there is no next, just do the idle thread. */
	if (New == nullptr)
	{
		New = this->IdleThread;
	}

	/* Don't bother trying to switching threads if we don't have to. */
	New->Lock();
	if (New == Old)
	{
		New->Unlock();
		pantheon::GlobalScheduler::Unlock();
		return;
	}

	Old->Lock();
	/* If it's not currently waiting, definitely don't. */
	if (New->MyState() != pantheon::Thread::STATE_WAITING)
	{
		New->Unlock();
		Old->Unlock();
		pantheon::GlobalScheduler::Unlock();
		return;
	}

	pantheon::ScopedLocalSchedulerLock _L;
	Old->SetState(pantheon::Thread::STATE_WAITING);
	Old->RefreshTicks();


	pantheon::Process *NewProc = New->MyProc();
	pantheon::Process::Switch(NewProc);
	this->CurThread = New;
	this->CurThread->SetState(pantheon::Thread::STATE_RUNNING);

	pantheon::CpuContext *OldContext = Old->GetRegisters();
	pantheon::CpuContext *NewContext = New->GetRegisters();

	/* Update the Thread Local Area register */
	pantheon::ipc::SetThreadLocalRegion(New->GetThreadLocalAreaRegister());

	/* TODO: Make this better */
	pantheon::GlobalScheduler::AppendIntoReadyList(Old);
	pantheon::GlobalScheduler::Unlock();

	Old->Unlock();
	New->Unlock();

	pantheon::Sync::DSBISH();
	pantheon::Sync::ISB();
	cpu_switch(OldContext, NewContext, CpuIRegOffset);
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	OBJECT_SELF_ASSERT();
	if (this->CurThread)
	{
		return this->CurThread->MyProc();
	}
	return nullptr;
}

pantheon::Thread *pantheon::Scheduler::MyThread()
{
	OBJECT_SELF_ASSERT();
	return this->CurThread;
}

extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);

/**
 * \~english @brief Creates a process, visible globally, from a name and address.
 * \~english @details A process is created, such that it can be run on any
 * available processor on the system. The given process name identifies the
 * process for debugging purposes and as a user-readable way to identify a
 * process, and the start address is a pointer to the first instruction the
 * initial threaq should execute when scheduled.
 * \~english @param[in] ProcStr A human-readable name for the process
 * \~english @param[in] StartAddr The initial value of the program counter
 * \~english @return TRUE is the process was sucessfully created, false otherwise.
 * \~english @author Brian Schnepp
 */
UINT32 pantheon::GlobalScheduler::CreateProcess(const pantheon::String &ProcStr, void *StartAddr)
{
	pantheon::ScopedGlobalSchedulerLock _L;
	
	pantheon::Process *NewProc = Process::Create();
	pantheon::ProcessCreateInfo Info = {};
	Info.Name = ProcStr;
	Info.EntryPoint = (pantheon::vmm::VirtualAddress)StartAddr;

	if (NewProc == nullptr)
	{
		return 0;
	}

	UINT32 Result = 0;
	{
		pantheon::ScopedLock _LL(NewProc);
		NewProc->Initialize(Info);
		Result = NewProc->ProcessID();
	}

	GlobalScheduler::ProcessList.PushFront(NewProc);

	if (NewProc != GlobalScheduler::ProcessList.Front())
	{
		StopError("NewProc was not front.");
	}

	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThreadCommon(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	pantheon::Thread *T = pantheon::Thread::Create();
	T->Initialize(Proc, StartAddr, ThreadData, Priority, TRUE);
	T->SetupThreadLocalArea();
	GlobalScheduler::ThreadList.PushFront(T);
	GlobalScheduler::AppendIntoReadyList(T);
	return GlobalScheduler::ThreadList.Front();
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThread(UINT32 PID, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	pantheon::ScopedGlobalSchedulerLock _L;

	pantheon::Process *SelProc = nullptr;
	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			SelProc = &Proc;
			break;
		}
	}

	pantheon::Thread *Result = nullptr;
	if (SelProc)
	{
		Result = GlobalScheduler::CreateUserThreadCommon(SelProc, StartAddr, ThreadData, Priority);
	}
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	pantheon::ScopedGlobalSchedulerLock _L;
	pantheon::Thread *Result = pantheon::GlobalScheduler::CreateUserThreadCommon(Proc, StartAddr, ThreadData, Priority);
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority)
{
	/* Assert that AccessSpinlock is locked. */
	if (GlobalScheduler::AccessSpinlock.IsLocked() == FALSE)
	{
		StopError("CreateThread without AccessSpinlock");
	}

	pantheon::Thread *T = pantheon::Thread::Create();
	T->Initialize(Proc, StartAddr, ThreadData, Priority, FALSE);
	GlobalScheduler::ThreadList.PushFront(T);
	GlobalScheduler::AppendIntoReadyList(T);
	return GlobalScheduler::ThreadList.Front();
}

void pantheon::GlobalScheduler::Lock()
{
	AccessSpinlock.Acquire();
}

void pantheon::GlobalScheduler::Unlock()
{
	AccessSpinlock.Release();
}

pantheon::Atomic<BOOL> pantheon::GlobalScheduler::Okay;
pantheon::Spinlock pantheon::GlobalScheduler::AccessSpinlock;

pantheon::LinkedList<pantheon::Process> pantheon::GlobalScheduler::ProcessList;
pantheon::LinkedList<pantheon::Thread> pantheon::GlobalScheduler::ThreadList;

pantheon::Thread *pantheon::GlobalScheduler::ReadyHead;
pantheon::Thread *pantheon::GlobalScheduler::ReadyTail;

static pantheon::Process IdleProc;
VOID pantheon::GlobalScheduler::Init()
{
	IdleProc = pantheon::Process();

	GlobalScheduler::ReadyHead = nullptr;
	GlobalScheduler::ReadyTail = nullptr;

	GlobalScheduler::ThreadList = LinkedList<Thread>();
	GlobalScheduler::ProcessList = LinkedList<Process>();

	GlobalScheduler::AccessSpinlock = Spinlock("access_spinlock");
	GlobalScheduler::ProcessList.PushFront(&IdleProc);
	GlobalScheduler::Okay.Store(TRUE);
}

pantheon::Thread *pantheon::GlobalScheduler::CreateProcessorIdleThread()
{
	while (!GlobalScheduler::Okay.Load()){}

	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			/* Note the idle thread needs to have no meaningful data: it gets smashed on startup.  */
			pantheon::Thread *CurThread = Thread::Create();
			CurThread->Initialize(&Proc, nullptr, nullptr, pantheon::Thread::PRIORITY_VERYLOW, FALSE);
			return CurThread;
		}
	}
	GlobalScheduler::AccessSpinlock.Release();
	return nullptr;
}

UINT64 pantheon::GlobalScheduler::CountThreads(UINT64 PID)
{
	UINT64 Count = 0;
	pantheon::GlobalScheduler::Lock();
	for (const pantheon::Thread &T : GlobalScheduler::ThreadList)
	{
		if (T.MyProc()->ProcessID() == PID)
		{
			Count++;
		}
	}
	pantheon::GlobalScheduler::Unlock();
	return Count;
}

UINT32 pantheon::AcquireProcessID()
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

UINT64 pantheon::AcquireThreadID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT32 RetVal = 0;
	static UINT64 ThreadID = 0;
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ThreadID++;
	return RetVal;
}

void pantheon::AttemptReschedule()
{
	pantheon::Scheduler *CurSched = pantheon::CPU::GetCurSched();
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

		CurSched->Reschedule();
	}
}

extern "C" VOID FinishThread()
{
	pantheon::CPU::GetCurThread()->EnableScheduling();
}

BOOL pantheon::GlobalScheduler::MapPages(UINT32 PID, pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages)
{
	BOOL Success = FALSE;
	pantheon::GlobalScheduler::Lock();
	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			pantheon::ScopedLock L(&Proc);
			for (UINT64 Index = 0; Index < NumPages; ++Index)
			{
				Proc.MapAddress(VAddresses[Index], PAddresses[Index], PageAttributes);
			}
			Success = TRUE;
			break;
		}
	}
	pantheon::GlobalScheduler::Unlock();
	return Success;
}

BOOL pantheon::GlobalScheduler::RunProcess(UINT32 PID)
{
	BOOL Success = FALSE;
	pantheon::vmm::VirtualAddress Entry = 0x00;
	pantheon::GlobalScheduler::Lock();
	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			Proc.Lock();
			Proc.SetState(pantheon::Process::STATE_RUNNING);
			Entry = Proc.GetEntryPoint();
			Success = TRUE;
			Proc.Unlock();
			break;
		}
	}
	pantheon::GlobalScheduler::Unlock();

	if (Success)
	{
		pantheon::GlobalScheduler::CreateUserThread(PID, (void*)(Entry), nullptr);
	}
	return Success;	
}

BOOL pantheon::GlobalScheduler::SetState(UINT32 PID, pantheon::Process::State State)
{
	BOOL Success = FALSE;
	pantheon::GlobalScheduler::Lock();
	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			Proc.Lock();
			Proc.SetState(State);
			Success = TRUE;
			Proc.Unlock();
			break;
		}
	}
	pantheon::GlobalScheduler::Unlock();
	return Success;	
}

void pantheon::GlobalScheduler::AppendIntoReadyList(pantheon::Thread *Next)
{
	/* Make sure we're locked before doing this... */
	if (GlobalScheduler::AccessSpinlock.IsLocked() == FALSE)
	{
		StopError("Appending into readylist while not locked");
	}

	/* If this thread is null for whatever reason, don't bother. */
	if (Next == nullptr)
	{
		return;
	}

	if (GlobalScheduler::ReadyTail)
	{
		GlobalScheduler::ReadyTail->SetNext(Next);
		GlobalScheduler::ReadyTail = Next;
	}
	else
	{
		/* Only possible if the queue really is empty. */
		GlobalScheduler::ReadyTail = Next;
		GlobalScheduler::ReadyHead = Next;
	}
	GlobalScheduler::ReadyTail->SetNext(nullptr);
}

pantheon::Thread *pantheon::GlobalScheduler::PopFromReadyList()
{
	/* Make sure we're locked before doing this... */
	if (GlobalScheduler::AccessSpinlock.IsLocked() == FALSE)
	{
		StopError("Poping from readylist while not locked");
	}

	pantheon::Thread *Head = GlobalScheduler::ReadyHead;
	if (Head)
	{
		GlobalScheduler::ReadyHead = GlobalScheduler::ReadyHead->Next();
	}

	if (GlobalScheduler::ReadyHead == nullptr)
	{
		GlobalScheduler::ReadyTail = nullptr;
	}
	return Head;
}