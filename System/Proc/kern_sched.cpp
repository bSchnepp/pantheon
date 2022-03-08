#include <kern.h>
#include <stddef.h>

#include <arch.hpp>
#include <cpureg.hpp>
#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_cpu.hpp>
#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <System/PhyMemory/kern_alloc.hpp>
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

static void proc_idle()
{
	for (;;) 
	{
	}
}

/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::Scheduler::Scheduler()
{
	/* hackishly create an idle thread */
	UINT64 SP = (UINT64)((BasicMalloc(4096)())) + 4096;
	UINT64 IP = (UINT64)proc_idle;

	#ifdef POISON_MEMORY
	SetBufferBytes((CHAR*)SP-4096, 0xAF, 4096);
	#endif

	this->CurThread = pantheon::GetGlobalScheduler()->CreateProcessorIdleThread(SP, IP);
}

pantheon::Scheduler::~Scheduler()
{

}

extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset);

VOID pantheon::Scheduler::PerformCpuSwitch(Thread *Old, Thread *New)
{
	pantheon::CpuContext *Prev = (Old->GetRegisters());
	pantheon::CpuContext *Next = (New->GetRegisters());

	UINT64 NewTTBR0 = (UINT64)New->MyProc()->GetTTBR0();
	pantheon::CPUReg::W_TTBR0_EL1(NewTTBR0);

	New->Unlock();
	Old->Unlock();

	cpu_switch(Prev, Next, CpuIRegOffset);	
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
	/* Interrupts must be enabled before we can do anything. */
	if (pantheon::CPU::ICOUNT())
	{
		StopError("Try to reschedule with interrupts off");
	}

	this->CurThread->BlockScheduling();
	pantheon::Thread *Old = this->CurThread;
	pantheon::Thread *New = GlobalScheduler::AcquireThread();

	Old->Lock();
	Old->SetState(pantheon::THREAD_STATE_WAITING);

	this->CurThread = New;
	this->CurThread->Lock();

	this->PerformCpuSwitch(Old, New);
	this->CurThread->EnableScheduling();
}

pantheon::Process *pantheon::Scheduler::MyProc()
{
	if (this->CurThread)
	{
		return this->CurThread->MyProc();
	}
	return nullptr;
}

pantheon::Thread *pantheon::Scheduler::MyThread()
{
	return this->CurThread;
}

extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);

static void true_drop_process(void *StartAddress, pantheon::vmm::VirtualAddress StackAddr)
{
	/* This gets usermode for aarch64, needs to be abstracted! */
	drop_usermode((UINT64)StartAddress, 0x00, StackAddr);
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
 * \~english @return TRUE is the process was sucessfully created, false otherwise.
 * \~english @author Brian Schnepp
 */
UINT32 pantheon::GlobalScheduler::CreateProcess(const pantheon::String &ProcStr, void *StartAddr)
{
	pantheon::Process *NewProc = Process::Create();
	pantheon::ProcessCreateInfo Info = {};
	Info.Name = ProcStr;
	Info.EntryPoint = (pantheon::vmm::VirtualAddress)StartAddr;

	AccessSpinlock.Acquire();

	NewProc->Lock();
	NewProc->Initialize(Info);
	UINT32 Result = NewProc->ProcessID();
	NewProc->Unlock();

	GlobalScheduler::ProcessList.PushFront(NewProc);

	if (NewProc != GlobalScheduler::ProcessList.Front())
	{
		StopError("NewProc was not front.");
	}

	AccessSpinlock.Release();
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThreadCommon(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	/* Attempt 4 pages of stack space for now... */
	static constexpr UINT64 InitialThreadStackSize = pantheon::vmm::SmallestPageSize * 4;
	pantheon::Thread T(Proc);

	Optional<void*> StackSpace = BasicMalloc(InitialThreadStackSize);
	if (StackSpace.GetOkay())
	{
		UINT64 IStackSpace = (UINT64)StackSpace();
		#ifdef POISON_MEMORY
		SetBufferBytes((CHAR*)IStackSpace, 0xAF, InitialThreadStackSize);
		#endif
		IStackSpace += InitialThreadStackSize;
		pantheon::Thread *T = Thread::Create();
		*T = pantheon::Thread(Proc);

		T->Lock();

		UINT64 IStartAddr = (UINT64)true_drop_process;
		UINT64 IThreadData = (UINT64)ThreadData;

		pantheon::CpuContext *Regs = T->GetRegisters();
		Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
		Regs->x20 = (UINT64)StartAddr;
		Regs->x21 = pantheon::Process::StackAddr;
		T->SetState(pantheon::THREAD_STATE_WAITING);
		T->SetPriority(Priority);
		T->Unlock();

		GlobalScheduler::ThreadList.PushFront(T);
	}
	return GlobalScheduler::ThreadList.Front();
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThread(UINT32 PID, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	GlobalScheduler::AccessSpinlock.Acquire();
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
	GlobalScheduler::AccessSpinlock.Release();
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateUserThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	GlobalScheduler::AccessSpinlock.Acquire();
	pantheon::Thread *Result = pantheon::GlobalScheduler::CreateUserThreadCommon(Proc, StartAddr, ThreadData, Priority);
	GlobalScheduler::AccessSpinlock.Release();
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	/* Assert that AccessSpinlock is locked. */
	if (GlobalScheduler::AccessSpinlock.IsLocked() == FALSE)
	{
		StopError("CreateThread without AccessSpinlock");
	}

	/* Attempt 4 pages of stack space for now... */
	static constexpr UINT64 InitialThreadStackSize = pantheon::vmm::SmallestPageSize * 4;
	pantheon::Thread T(Proc);

	Optional<void*> StackSpace = BasicMalloc(InitialThreadStackSize);
	pantheon::Thread *Result = nullptr;
	if (StackSpace.GetOkay())
	{
		UINT64 IStackSpace = (UINT64)StackSpace();
		#ifdef POISON_MEMORY
		SetBufferBytes((CHAR*)IStackSpace, 0xAF, InitialThreadStackSize);
		#endif
		IStackSpace += InitialThreadStackSize;
		pantheon::Thread *T = Thread::Create();
		*T = pantheon::Thread(Proc);

		T->Lock();

		UINT64 IStartAddr = (UINT64)StartAddr;
		UINT64 IThreadData = (UINT64)ThreadData;

		pantheon::CpuContext *Regs = T->GetRegisters();
		Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
		T->SetState(pantheon::THREAD_STATE_WAITING);
		T->SetPriority(Priority);
		T->Unlock();

		GlobalScheduler::ThreadList.PushFront(T);
	}
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::AcquireThread()
{
	pantheon::Thread *ReturnValue = nullptr;
	while (ReturnValue == nullptr)
	{
		UINT64 MaxTicks = 0;
		GlobalScheduler::AccessSpinlock.Acquire();
		for (pantheon::Thread &Thr : GlobalScheduler::ThreadList)
		{
			pantheon::ScopedLock L(&Thr);
			pantheon::Process *Proc = Thr.MyProc();
			pantheon::ThreadState State = Thr.MyState();

			if (Thr.MyProc() == nullptr)
			{
				StopErrorFmt("Invalid process on thread: 0x%lx\n", Thr.ThreadID());
			}

			pantheon::ScopedLock L2(Proc);
			if (Proc->MyState() != pantheon::PROCESS_STATE_RUNNING)
			{
				continue;
			}

			if (State != pantheon::THREAD_STATE_WAITING)
			{
				continue;
			}

			UINT64 TickCount = Thr.TicksLeft();
			if (TickCount > MaxTicks)
			{
				MaxTicks = TickCount;
				
				/* If we had a previous state, make sure we release it. */
				if (ReturnValue)
				{
					ReturnValue->Lock();
					ReturnValue->SetState(pantheon::THREAD_STATE_WAITING);
					ReturnValue->Unlock();
				}

				ReturnValue = &Thr;
				ReturnValue->SetState(pantheon::THREAD_STATE_RUNNING);
			}
		}

		if (ReturnValue != nullptr)
		{
			GlobalScheduler::AccessSpinlock.Release();
			return ReturnValue;
		}

		/* Nothing was found: refresh everything, try again. */
		for (pantheon::Thread &Thr : GlobalScheduler::ThreadList)
		{
			pantheon::ScopedLock L(&Thr);
			Thr.RefreshTicks();
		}

		GlobalScheduler::AccessSpinlock.Release();
	}
	StopErrorFmt("Jumped outside acquire thread loop\n");
}

static pantheon::Process IdleProc;
VOID pantheon::GlobalScheduler::Init()
{
	IdleProc = pantheon::Process();

	GlobalScheduler::ThreadList = LinkedList<Thread>();
	GlobalScheduler::ProcessList = LinkedList<Process>();

	GlobalScheduler::AccessSpinlock = Spinlock("access_spinlock");
	GlobalScheduler::ProcessList.PushFront(&IdleProc);
	GlobalScheduler::Okay.Store(TRUE);
}

pantheon::Thread *pantheon::GlobalScheduler::CreateProcessorIdleThread(UINT64 SP, UINT64 IP)
{
	while (!GlobalScheduler::Okay.Load()){}

	GlobalScheduler::AccessSpinlock.Acquire();
	for (pantheon::Process &Proc : GlobalScheduler::ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			pantheon::Thread *CurThread = Thread::Create();
			*CurThread = Thread(&Proc, pantheon::THREAD_PRIORITY_NORMAL);
			CurThread->Lock();
			CurThread->GetRegisters()->SetSP(SP);
			CurThread->GetRegisters()->SetPC(IP);
			CurThread->SetState(pantheon::THREAD_STATE_RUNNING);
			CurThread->Unlock();

			GlobalScheduler::ThreadList.PushFront(CurThread);

			GlobalScheduler::AccessSpinlock.Release();
			return CurThread;
		}
	}
	GlobalScheduler::AccessSpinlock.Release();
	return nullptr;
}

UINT64 pantheon::GlobalScheduler::CountThreads(UINT64 PID)
{
	UINT64 Count = 0;
	AccessSpinlock.Acquire();
	for (const pantheon::Thread &T : GlobalScheduler::ThreadList)
	{
		if (T.MyProc()->ProcessID() == PID)
		{
			Count++;
		}
	}
	AccessSpinlock.Release();
	return Count;
}

static pantheon::GlobalScheduler GlobalSched;

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

pantheon::GlobalScheduler *pantheon::GetGlobalScheduler()
{
	return &GlobalSched;
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
	AccessSpinlock.Acquire();
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
	AccessSpinlock.Release();
	return Success;
}

BOOL pantheon::GlobalScheduler::SetState(UINT32 PID, pantheon::ProcessState State)
{
	BOOL Success = FALSE;
	AccessSpinlock.Acquire();
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
	AccessSpinlock.Release();
	return Success;	
}