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

	pantheon::GetGlobalScheduler()->ReleaseThread(Old);

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
	pantheon::Thread *New = pantheon::GetGlobalScheduler()->AcquireThread();

	Old->Lock();
	New->Lock();

	if (New == Old)
	{
		StopError("Same thread was issued to run");
	}

	this->CurThread = New;

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

pantheon::GlobalScheduler::GlobalScheduler()
{
	/* NYI */
	this->Init();
}

pantheon::GlobalScheduler::~GlobalScheduler()
{
	/* NYI */
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
UINT32 pantheon::GlobalScheduler::CreateProcess(pantheon::String ProcStr, void *StartAddr)
{
	pantheon::Thread *Value = nullptr;
	
	AccessSpinlock.Acquire();
	pantheon::Process *NewProc = this->ProcAllocator.AllocateNoCtor();
	*NewProc = Process(ProcStr);

	this->ProcessList.PushFront(NewProc);
	NewProc->Lock();
	Value = pantheon::GetGlobalScheduler()->CreateThread(NewProc, (VOID*)true_drop_process, nullptr);
	if (Value != nullptr)
	{
		/* TODO: Handle abstraction for different architectures */
		Value->Lock();
		Value->GetRegisters()->x20 = (UINT64)StartAddr;
		Value->GetRegisters()->x21 = (UINT64)pantheon::Process::StackAddr;
		Value->Unlock();
	}

	UINT32 Result = NewProc->ProcessID();
	NewProc->Unlock();
	AccessSpinlock.Release();
	
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
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
		Result = this->CreateThread(Proc, StartAddr, ThreadData, Priority, (void*)IStackSpace);
	}
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority, void *StackTop)
{
	pantheon::Thread *T = this->ThreadAllocator.AllocateNoCtor();
	*T = pantheon::Thread(Proc);

	T->Lock();

	UINT64 IStartAddr = (UINT64)StartAddr;
	UINT64 IThreadData = (UINT64)ThreadData;
	UINT64 IStackSpace = (UINT64)StackTop;

	pantheon::CpuContext *Regs = T->GetRegisters();
	Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
	T->SetState(pantheon::THREAD_STATE_WAITING);
	T->SetPriority(Priority);
	T->Unlock();

	this->ThreadList.PushFront(T);
	return T;
}


static pantheon::Process IdleProc;
VOID pantheon::GlobalScheduler::Init()
{
	this->ThreadList = LinkedList<Thread>();
	this->ProcessList = LinkedList<Process>();

	this->ProcAllocator = pantheon::mm::SlabCache<Process>(this->ArrayProcs, GlobalScheduler::NumProcs);
	this->ThreadAllocator = pantheon::mm::SlabCache<Thread>(this->ArrayThreads, GlobalScheduler::NumThreads);

	AccessSpinlock = Spinlock("access_spinlock");
	IdleProc = pantheon::Process();

	this->ProcessList.PushFront(&IdleProc);
	this->Okay.Store(TRUE);
}

pantheon::Thread *pantheon::GlobalScheduler::CreateProcessorIdleThread(UINT64 SP, UINT64 IP)
{
	while (!this->Okay.Load()){}

	this->AccessSpinlock.Acquire();
	for (pantheon::Process &Proc : this->ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			pantheon::Thread *CurThread = this->ThreadAllocator.AllocateNoCtor();
			*CurThread = Thread(&Proc, pantheon::THREAD_PRIORITY_NORMAL);
			CurThread->Lock();
			CurThread->GetRegisters()->SetSP(SP);
			CurThread->GetRegisters()->SetPC(IP);
			CurThread->SetState(pantheon::THREAD_STATE_RUNNING);
			CurThread->Unlock();

			this->ThreadList.PushFront(CurThread);

			this->AccessSpinlock.Release();
			return CurThread;
		}
	}
	this->AccessSpinlock.Release();
	return nullptr;
}


/**
 * \~english @brief Obtains a Process which has an inactive thread, or the idle process if none available.
 * \~english @return An instance of a Process to execute
 * \~english @author Brian Schnepp
 */
pantheon::Thread *pantheon::GlobalScheduler::AcquireThread()
{
	while (!this->Okay.Load()){}
	
	pantheon::Thread *Thr = nullptr;
	pantheon::CPU::PUSHI();
	while (Thr == nullptr)
	{
		UINT64 TListSize = this->ThreadList.Size();
		if (TListSize == 0)
		{
			break;
		}

		this->AccessSpinlock.Acquire();
		UINT64 MaxTicks = 0;
		for (pantheon::Thread &MaybeThr : this->ThreadList)
		{
			MaybeThr.Lock();
			if (MaybeThr.MyState() != pantheon::THREAD_STATE_WAITING)
			{
				MaybeThr.Unlock();
				continue;
			}

			pantheon::Process *ThrProc = MaybeThr.MyProc();
			if (ThrProc == nullptr)
			{
				StopErrorFmt("Invalid Process: %lx\n", ThrProc);
			}

			ThrProc->Lock();
			if (ThrProc->MyState() != pantheon::PROCESS_STATE_RUNNING)
			{
				ThrProc->Unlock();
				MaybeThr.Unlock();
				continue;
			}
			ThrProc->Unlock();

			UINT64 TickCount = MaybeThr.TicksLeft();
			pantheon::ThreadState State = MaybeThr.MyState();

			if (State == pantheon::THREAD_STATE_WAITING && TickCount > MaxTicks)
			{
				MaxTicks = MaybeThr.TicksLeft();
				if (Thr)
				{
					Thr->Lock();
					Thr->SetState(pantheon::THREAD_STATE_WAITING);
					Thr->Unlock();
				}
				Thr = &MaybeThr;
				Thr->SetState(pantheon::THREAD_STATE_RUNNING);
			}
			MaybeThr.Unlock();
		}

		if (Thr)
		{
			this->AccessSpinlock.Release();
			break;
		}
		
		/* If we didn't find one after all that, refresh everyone. */
		for (pantheon::Thread &MaybeThr : this->ThreadList)
		{
			MaybeThr.Lock();
			MaybeThr.RefreshTicks();
			MaybeThr.Unlock();
		}
		this->AccessSpinlock.Release();
	}
	pantheon::CPU::POPI();
	return Thr;
}

UINT64 pantheon::GlobalScheduler::CountThreads(UINT64 PID)
{
	UINT64 Count = 0;
	AccessSpinlock.Acquire();
	for (const pantheon::Thread &T : this->ThreadList)
	{
		if (T.MyProc()->ProcessID() == PID)
		{
			Count++;
		}
	}
	AccessSpinlock.Release();
	return Count;
}

void pantheon::GlobalScheduler::ReleaseThread(Thread *T)
{
	/* T->Lock must be held. */
	if (T->IsLocked() == FALSE)
	{
		StopError("releasing thread without lock");
	}
	T->SetState(pantheon::THREAD_STATE_WAITING);
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
	pantheon::CPU::PUSHI();
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ProcessID++;
	pantheon::CPU::POPI();
	return RetVal;
}

UINT64 pantheon::AcquireThreadID()
{
	/* TODO: When we run out of IDs, go back and ensure we don't
	 * reuse an ID already in use!
	 */
	UINT32 RetVal = 0;
	static UINT64 ThreadID = 0;
	pantheon::CPU::PUSHI();
	/* A copy has to be made since we haven't unlocked the spinlock yet. */
	RetVal = ThreadID++;
	pantheon::CPU::POPI();
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
	for (pantheon::Process &Proc : this->ProcessList)
	{
		if (Proc.ProcessID() == PID)
		{
			Proc.MapAddresses(VAddresses, PAddresses, PageAttributes, NumPages);
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
	for (pantheon::Process &Proc : this->ProcessList)
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