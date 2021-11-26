#include <stddef.h>

#include <arch.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_cpu.hpp>
#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <System/PhyMemory/kern_alloc.hpp>

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
		pantheon::CPU::GetCurSched()->MaybeReschedule();
	}
}

/**
 * \~english @brief Initalizes an instance of a per-core scheduler.
 * \~english @author Brian Schnepp
 */
pantheon::Scheduler::Scheduler()
{
	this->ShouldReschedule.Store(FALSE);
	this->IgnoreReschedule.Store(FALSE);

	/* hackishly create an idle thread */
	UINT64 SP = (UINT64)((BasicMalloc(4096)())) + 4096;
	UINT64 IP = (UINT64)proc_idle;

	this->CurThread = static_cast<pantheon::Thread*>(BasicMalloc(sizeof(pantheon::Thread))());
	*this->CurThread = pantheon::Thread(pantheon::GetGlobalScheduler()->ObtainProcessByID(0), pantheon::THREAD_PRIORITY_NORMAL);

	this->CurThread->GetRegisters()->SetSP(SP);
	this->CurThread->GetRegisters()->SetPC(IP);
	this->CurThread->SetState(pantheon::THREAD_STATE_RUNNING);
}

pantheon::Scheduler::~Scheduler()
{

}

extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset);

VOID pantheon::Scheduler::PerformCpuSwitch(Thread *Old, Thread *New)
{
	pantheon::CpuContext *Prev = (Old->GetRegisters());
	pantheon::CpuContext *Next = (New->GetRegisters());

	this->ShouldReschedule.Store(FALSE);
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
	if (this->IgnoreReschedule.Load() == TRUE)
	{
		return;
	}

	/* Interrupts must be enabled before we can do anything. */
	if (pantheon::CPU::ICOUNT())
	{
		return;
	}

	this->IgnoreReschedule.Store(TRUE);
	pantheon::Thread *Old = this->CurThread;
	pantheon::Thread *New = pantheon::GetGlobalScheduler()->AcquireThread();

	if (New == nullptr)
	{
		pantheon::CPU::HLT();
		return;
	}

	this->CurThread = New;
	pantheon::GetGlobalScheduler()->ReleaseThread(Old);
	this->PerformCpuSwitch(Old, New);
	this->IgnoreReschedule.Store(FALSE);
	pantheon::CPU::STI();
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

/**
 * \~english @brief Attempts to reschedule if the appropriate flag is fired
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::MaybeReschedule()
{
	if (this->ShouldReschedule.Load() == TRUE)
	{
		this->Reschedule();
	}
}

/**
 * \~english @brief Signals that this scheduler should run a different thread
 * \~english @author Brian Schnepp
 */
void pantheon::Scheduler::SignalReschedule()
{
	this->ShouldReschedule.Store(TRUE);
}

void pantheon::Scheduler::StopPremption()
{
	this->IgnoreReschedule.Store(TRUE);
}

void pantheon::Scheduler::EnablePremption()
{
	this->IgnoreReschedule.Store(FALSE);
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
BOOL pantheon::GlobalScheduler::CreateProcess(pantheon::String ProcStr, void *StartAddr)
{
	BOOL Value = FALSE;
	
	AccessSpinlock.Acquire();
	UINT64 Index = this->ProcessList.Size();
	pantheon::Process NewProc(ProcStr);
	this->ProcessList.Add(NewProc);
	Value = this->ProcessList[Index].CreateThread(StartAddr, nullptr);
	AccessSpinlock.Release();
	
	return Value;
}

BOOL pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	pantheon::Thread T(Proc);

	/* Attempt 128KB of stack space for now... */
	UINT64 StackSz = 4096;
	Optional<void*> StackSpace = BasicMalloc(StackSz);
	if (StackSpace.GetOkay())
	{
		UINT64 IStackSpace = (UINT64)StackSpace();
		IStackSpace += StackSz;
		this->CreateThread(Proc, StartAddr, ThreadData, Priority, (void*)IStackSpace);
	}
	return StackSpace.GetOkay();
}

BOOL pantheon::GlobalScheduler::CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority, void *StackTop)
{
	pantheon::Thread T(Proc);

	UINT64 IStartAddr = (UINT64)StartAddr;
	UINT64 IThreadData = (UINT64)ThreadData;
	UINT64 IStackSpace = (UINT64)StackTop;

	pantheon::CpuContext *Regs = T.GetRegisters();
	Regs->SetInitContext(IStartAddr, IThreadData, IStackSpace);
	T.SetState(pantheon::THREAD_STATE_WAITING);
	T.SetPriority(Priority);
	this->ThreadList.Add(pantheon::Thread(T));
	return TRUE;
}

VOID pantheon::GlobalScheduler::Init()
{
	this->ThreadList = ArrayList<Thread>();
	this->ProcessList = ArrayList<Process>();
	AccessSpinlock = Spinlock("access_spinlock");

	pantheon::Process Idle;
	this->ProcessList.Add(Idle);
}

VOID pantheon::GlobalScheduler::CreateIdleProc(void *StartAddr)
{
	AccessSpinlock.Acquire();
	for (pantheon::Process &Proc : this->ProcessList)
	{
		if (Proc.ProcessID() == 0)
		{
			Proc.CreateThread(StartAddr, nullptr);
			break;
		}
	}
	AccessSpinlock.Release();
}


/**
 * \~english @brief Obtains a Process which has an inactive thread, or the idle process if none available.
 * \~english @return An instance of a Process to execute
 * \~english @author Brian Schnepp
 */
pantheon::Thread *pantheon::GlobalScheduler::AcquireThread()
{
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
			if (MaybeThr.IsLocked() || MaybeThr.MyState() == pantheon::THREAD_STATE_RUNNING)
			{
				continue;
			}

			MaybeThr.Lock();
			UINT64 TickCount = MaybeThr.TicksLeft();
			pantheon::ThreadState State = MaybeThr.MyState();

			if (State == pantheon::THREAD_STATE_WAITING && TickCount > MaxTicks)
			{
				MaxTicks = MaybeThr.TicksLeft();
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
			if (MaybeThr.IsLocked() || MaybeThr.MyState() == pantheon::THREAD_STATE_RUNNING)
			{
				continue;
			}

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
	T->Lock();
	T->SetState(pantheon::THREAD_STATE_WAITING);
	T->Unlock();
}

pantheon::Process *pantheon::GlobalScheduler::ObtainProcessByID(UINT64 PID)
{
	for (pantheon::Process &Proc : this->ProcessList)
	{
		/* TODO: lock Proc */
		if (Proc.ProcessID() == PID)
		{
			return &Proc;
		}
	}
	return nullptr;
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
	pantheon::CPU::CoreInfo *CoreData = pantheon::CPU::GetCoreInfo();
	pantheon::Scheduler *CurSched = CoreData->CurSched;
	pantheon::Thread *CurThread = CurSched->MyThread();

	UINT64 RemainingTicks = 0;
	if (CurThread)
	{
		CurThread->CountTick();
		RemainingTicks = CurThread->TicksLeft();
	}
		
	if (RemainingTicks == 0)
	{
		CurSched->SignalReschedule();
	}

	CurSched->MaybeReschedule();
}