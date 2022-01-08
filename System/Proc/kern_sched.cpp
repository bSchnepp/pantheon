#include <stddef.h>

#include <arch.hpp>
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
pantheon::Process *pantheon::GlobalScheduler::CreateProcess(pantheon::String ProcStr, void *StartAddr)
{
	pantheon::Thread *Value = nullptr;
	
	AccessSpinlock.Acquire();
	UINT64 Index = this->ProcessList.Size();
	pantheon::Process NewProc(ProcStr);
	this->ProcessList.Add(NewProc);
	this->ProcessList[Index].Lock();
	Value = this->ProcessList[Index].CreateThread(StartAddr, nullptr);
	this->ProcessList[Index].Unlock();
	AccessSpinlock.Release();
	
	if (Value)
	{
		return &this->ProcessList[Index];
	}
	return nullptr;
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
		IStackSpace += InitialThreadStackSize;
		Result = this->CreateThread(Proc, StartAddr, ThreadData, Priority, (void*)IStackSpace);
	}
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority, void *StackTop)
{
	Optional<void*> MaybeMem = BasicMalloc(sizeof(pantheon::Thread));
	if (MaybeMem.GetOkay() == FALSE)
	{
		return FALSE;
	}

	void *Mem = MaybeMem.GetValue();
	pantheon::Thread *T = static_cast<pantheon::Thread*>(Mem);
	*T = pantheon::Thread(Proc);
	T->Lock();

	UINT64 IStartAddr = (UINT64)StartAddr;
	UINT64 IThreadData = (UINT64)ThreadData;
	UINT64 IStackSpace = (UINT64)StackTop;

	pantheon::CpuContext *Regs = T->GetRegisters();
	Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
	T->SetState(pantheon::THREAD_STATE_WAITING);
	T->SetPriority(Priority);
	this->ThreadList.Append(pantheon::LinkedList<pantheon::Thread>::CreateEntry(T));
	T->Unlock();
	return T;
}

VOID pantheon::GlobalScheduler::Init()
{
	this->ThreadList = pantheon::LinkedList<Thread>();
	this->ProcessList = ArrayList<Process>();
	AccessSpinlock = Spinlock("access_spinlock");

	pantheon::Process Idle;
	this->ProcessList.Add(Idle);
	this->Okay.Store(TRUE);
}

pantheon::Thread *pantheon::GlobalScheduler::CreateProcessorIdleThread(UINT64 SP, UINT64 IP)
{
	while (!this->Okay.Load()){}

	pantheon::Thread *CurThread = static_cast<pantheon::Thread*>(BasicMalloc(sizeof(pantheon::Thread))());
	*CurThread = pantheon::Thread(pantheon::GetGlobalScheduler()->ObtainProcessByID(0), pantheon::THREAD_PRIORITY_NORMAL);
	CurThread->Lock();
	CurThread->GetRegisters()->SetSP(SP);
	CurThread->GetRegisters()->SetPC(IP);
	CurThread->SetState(pantheon::THREAD_STATE_RUNNING);
	CurThread->Unlock();
	this->AccessSpinlock.Acquire();
	this->ThreadList.Append(pantheon::LinkedList<pantheon::Thread>::CreateEntry(CurThread));
	this->AccessSpinlock.Release();
	return CurThread;
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

		UINT64 MaxTicks = 0;
		for (pantheon::Thread &MaybeThr : this->ThreadList)
		{
			MaybeThr.Lock();
			if (MaybeThr.MyState() != pantheon::THREAD_STATE_WAITING)
			{
				MaybeThr.Unlock();
				continue;
			}

			if (MaybeThr.MyProc()->MyState() != pantheon::PROCESS_STATE_RUNNING)
			{
				MaybeThr.Unlock();
				continue;
			}

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
			break;
		}
		
		/* If we didn't find one after all that, refresh everyone. */
		for (pantheon::Thread &MaybeThr : this->ThreadList)
		{
			MaybeThr.Lock();
			MaybeThr.RefreshTicks();
			MaybeThr.Unlock();
		}
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

pantheon::Process *pantheon::GlobalScheduler::ObtainProcessByID(UINT64 PID)
{
	pantheon::Process *Result = nullptr;
	for (pantheon::Process &Proc : this->ProcessList)
	{
		Proc.Lock();
		if (Proc.ProcessID() == PID)
		{
			Result = &Proc;
		}
		Proc.Unlock();

		if (Result)
		{
			break;
		}
	}
	return Result;
}

pantheon::Thread *pantheon::GlobalScheduler::ObtainThreadByID(UINT64 TID)
{
	for (pantheon::Thread &Thr : this->ThreadList)
	{
		Thr.Lock();
		if (Thr.ThreadID() == TID)
		{
			Thr.Unlock();
			return &Thr;
		}
		Thr.Unlock();
	}
	return nullptr;
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