#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <System/Proc/kern_proc.hpp>

#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_slab.hpp>
#include <Common/Structures/kern_linkedlist.hpp>

#ifndef _KERN_SCHED_HPP_
#define _KERN_SCHED_HPP_

namespace pantheon
{

class Scheduler
{

public:
	Scheduler();
	~Scheduler();

	void Reschedule();
	Process *MyProc();
	Thread *MyThread();

private:
	VOID PerformCpuSwitch(pantheon::CpuContext *Old, pantheon::CpuContext *New);
	Thread *CurThread;
	Thread *IdleThread;
};

namespace GlobalScheduler
{
	void Init();

	UINT32 CreateProcess(const pantheon::String &ProcStr, void *StartAddr);
	pantheon::Thread *CreateThread(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority = pantheon::Thread::PRIORITY_NORMAL);

	UINT64 CountThreads(UINT64 PID);

	BOOL RunProcess(UINT32 PID);
	BOOL SetState(UINT32 PID, pantheon::Process::State State);
	BOOL MapPages(UINT32 PID, pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

	void Lock();
	void Unlock();
};

class ScopedGlobalSchedulerLock
{
public:
	FORCE_INLINE ScopedGlobalSchedulerLock() { pantheon::GlobalScheduler::Lock(); }
	FORCE_INLINE ~ScopedGlobalSchedulerLock() { pantheon::GlobalScheduler::Unlock(); }	
};

class ScopedLocalSchedulerLock
{
public:
	FORCE_INLINE ScopedLocalSchedulerLock() { pantheon::CPU::GetCurThread()->BlockScheduling(); }
	FORCE_INLINE ~ScopedLocalSchedulerLock() { pantheon::CPU::GetCurThread()->EnableScheduling(); }	
};

UINT32 AcquireProcessID();
UINT64 AcquireThreadID();

void AttemptReschedule();

}

#endif