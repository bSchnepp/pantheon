#include <kern_datatypes.hpp>
#include <Proc/kern_cpu.hpp>

#include "Syscalls.hpp"
#include <vmm/vmm.hpp>

#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

#include <IPC/kern_port.hpp>
#include <IPC/kern_event.hpp>

template<typename T>
static T ReadArgument(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<T>(Val);
}


static const CHAR *ReadArgumentAsCharPointer(UINT64 Val)
{
	/* Don't allow reading of kernel memory */
	if (Val > pantheon::vmm::HigherHalfAddress)
	{
		return nullptr;
	}

	/* Make sure we translate this to something in kernel memory, 
	 * just to be safe. */
	pantheon::vmm::VirtualAddress RawPtr = Val;
	const CHAR *Result = nullptr;

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	CurThread->Lock();

	pantheon::Process *CurProc = CurThread->MyProc();
	CurProc->Lock();

	pantheon::vmm::PhysicalAddress PAddr = pantheon::vmm::VirtualToPhysicalAddress(CurProc->GetPageTable(), RawPtr);
	pantheon::vmm::VirtualAddress VAddr = pantheon::vmm::PhysicalToVirtualAddress(PAddr);

	Result = reinterpret_cast<const char *>(VAddr);

	CurProc->Unlock();
	CurThread->Unlock();

	return Result;
}

template<typename T>
static T *ReadArgumentAsPointer(UINT64 Val)
{
	/* Don't allow reading of kernel memory */
	if (Val > pantheon::vmm::HigherHalfAddress)
	{
		return nullptr;
	}

	/* Make sure we translate this to something in kernel memory, 
	 * just to be safe. */
	pantheon::vmm::VirtualAddress RawPtr = Val;
	T *Result = nullptr;

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	CurThread->Lock();

	pantheon::Process *CurProc = CurThread->MyProc();
	CurProc->Lock();

	pantheon::vmm::PhysicalAddress PAddr = pantheon::vmm::VirtualToPhysicalAddress(CurProc->GetPageTable(), RawPtr);
	pantheon::vmm::VirtualAddress VAddr = pantheon::vmm::PhysicalToVirtualAddress(PAddr);

	Result = reinterpret_cast<T*>(VAddr);

	CurProc->Unlock();
	CurThread->Unlock();

	return Result;	


}

static UINT64 ReadArgumentAsInteger(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<UINT64>(Val);
}

VOID pantheon::SVCExitProcess(pantheon::TrapFrame *CurFrame)
{
	PANTHEON_UNUSED(CurFrame);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	CurThread->MyProc()->Lock();
	CurThread->MyProc()->SetState(pantheon::Process::STATE_ZOMBIE);
	CurThread->MyProc()->Unlock();

	CurThread->Lock();
	CurThread->SetState(pantheon::Thread::STATE_TERMINATED);
	CurThread->SetState(pantheon::Thread::STATE_DEAD);
	CurThread->Unlock();
	
	pantheon::CPU::GetCoreInfo()->CurSched->Reschedule();
}

pantheon::Result pantheon::SVCForkProcess(pantheon::TrapFrame *CurFrame)
{
	/* nyi */
	PANTHEON_UNUSED(CurFrame);
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCLogText(pantheon::TrapFrame *CurFrame)
{
	const CHAR *Data = nullptr;
	
	Data = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	if (Data == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::CPU::GetCurThread()->Lock();
	SERIAL_LOG("%s\n", Data);
	pantheon::CPU::GetCurThread()->Unlock();
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCAllocateBuffer(pantheon::TrapFrame *CurFrame)
{
	UINT64 Sz = 0;
	
	Sz = ReadArgumentAsInteger(CurFrame->GetIntArgument(0));

	PANTHEON_UNUSED(Sz);

	/* NYI */
	return pantheon::Result::SYS_OK;
}

typedef void (*ThreadStartPtr)(void*);

/**
 * \~english @brief Creates a new thread for the given process.
 * \~english @param Entry The entry point for the new thread
 * \~english @param RESERVED Reserved space for future usage: will be for thread-local data. This space is reserved to ensure ABI compatibility in the future.
 * \~english @param StackTop The top of the stack for the newly made process
 * \~english @param Priority The priority of the new thread
 */
pantheon::Result pantheon::SVCCreateThread(pantheon::TrapFrame *CurFrame)
{
	/* Not yet implemented! */
	PANTHEON_UNUSED(CurFrame);
	return pantheon::Result::SYS_OK;
}


/* Note that all these signal syscalls aren't quite safe yet.
 * Some more work needs to be done to guarantee thread safety, no use-after-free, etc.
 */

/**
 * \~english @brief Creates a new named event.
 * \~english @param[in] Name The name of the event to create
 * \~english @param[out] WriteHandle The handle of the process for the write area
 * \~english @param[out] ReadHandle The handle of the process for the read area
 */
pantheon::Result pantheon::SVCCreateNamedEvent(pantheon::TrapFrame *CurFrame)
{
	const CHAR *Name = nullptr;
	Name = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	
	INT32 *ReadHandle = nullptr;
	ReadHandle = ReadArgument<INT32*>(CurFrame->GetIntArgument(1));

	INT32 *WriteHandle = nullptr;
	WriteHandle = ReadArgument<INT32*>(CurFrame->GetIntArgument(2));

	if (Name == nullptr || ReadHandle == nullptr || WriteHandle == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}


	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);
	pantheon::String EvtName(Name);

	pantheon::ipc::NamedEvent *Evt = pantheon::ipc::LookupEvent(EvtName);
	if (Evt != nullptr)
	{
		UINT8 Write = Proc->EncodeHandle(pantheon::Handle(Evt->Writable));
		UINT8 Read = Proc->EncodeHandle(pantheon::Handle(Evt->Readable));

		*WriteHandle = Write;
		*ReadHandle = Read;
		return pantheon::Result::SYS_OK;
	}

	Evt = pantheon::ipc::CreateNamedEvent(EvtName, Proc);
	if (Evt != nullptr)
	{
		/* TODO: Use copyin/copyout functions to safely read/write 
		 * to user memory */
		UINT8 Write = Proc->EncodeHandle(pantheon::Handle(Evt->Writable));
		UINT8 Read = Proc->EncodeHandle(pantheon::Handle(Evt->Readable));

		*WriteHandle = Write;
		*ReadHandle = Read;
		return pantheon::Result::SYS_OK;
	}
	return pantheon::Result::SYS_FAIL;
}

pantheon::Result pantheon::SVCSignalEvent(pantheon::TrapFrame *CurFrame)
{
	INT32 WriteHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));


	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::ScopedLock ScopeLockThread(CurThread);
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
	if (Hand == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	if (Hand->GetType() != pantheon::HANDLE_TYPE_WRITE_SIGNAL)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::WritableEvent *Evt = Hand->GetContent().WriteEvent;
	
	if (Evt)
	{
		Evt->Signaler = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_SIGNALED;
	}

	/* TODO: wakeup every process waiting on it */
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCClearEvent(pantheon::TrapFrame *CurFrame)
{
	INT32 WriteHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::ScopedLock ScopeLockThread(CurThread);
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
	if (Hand == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_WRITE_SIGNAL)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::WritableEvent *Evt = Hand->GetContent().WriteEvent;

	if (Evt)
	{
		Evt->Parent->Readable->Clearer = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_UNSIGNALED;
	}
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCResetEvent(pantheon::TrapFrame *CurFrame)
{
	INT32 ReadHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::ScopedLock ScopeLockThread(CurThread);
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(ReadHandle);
	if (Hand == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_READ_SIGNAL)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::ReadableEvent *Evt = Hand->GetContent().ReadEvent;
	if (Evt)
	{
		Evt->Clearer = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_UNSIGNALED;
		return pantheon::Result::SYS_OK;
	}
	return pantheon::Result::SYS_FAIL;
}

pantheon::Result pantheon::SVCPollEvent(pantheon::TrapFrame *CurFrame)
{
	INT32 Handle = static_cast<INT32>(CurFrame->GetIntArgument(0));

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockThread(CurThread);
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(Handle);
	if (Hand == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_READ_SIGNAL)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::ReadableEvent *Evt = Hand->GetContent().ReadEvent;
	if (Evt)
	{
		/* FIXME: Express this in a better way! */
		BOOL Result = Evt->Parent->Status == pantheon::ipc::EVENT_TYPE_SIGNALED;
		if (Result)
		{
			return (pantheon::Result)1;
		}
		return (pantheon::Result)0;
	}
	return pantheon::Result::SYS_FAIL;
}

pantheon::Result pantheon::SVCYield(pantheon::TrapFrame *CurFrame)
{
	PANTHEON_UNUSED(CurFrame);
	pantheon::Scheduler *CurSched = pantheon::CPU::GetCurSched();
	{
		pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
		pantheon::ScopedLock ScopeLockThread(CurThread);
		CurThread->SetTicks(0);
	}
	CurSched->Reschedule();
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCExitThread(pantheon::TrapFrame *CurFrame)
{
	PANTHEON_UNUSED(CurFrame);
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Process *CurProc = CurThread->MyProc();
	pantheon::Scheduler *CurSched = pantheon::CPU::GetCurSched();
	CurProc->Lock();
	CurThread->Lock();
	CurThread->SetState(pantheon::Thread::STATE_TERMINATED);
	CurThread->Unlock();

	if (pantheon::GlobalScheduler::CountThreads(CurThread->MyProc()->ProcessID()) == 0)
	{
		CurProc->SetState(pantheon::Process::STATE_ZOMBIE);
	}
	CurProc->Unlock();
	CurSched->Reschedule();
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCExecute(pantheon::TrapFrame *CurFrame)
{
	INT32 Handle = static_cast<INT32>(CurFrame->GetIntArgument(0));
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();	
	CurThread->Lock();
	Proc->Lock();

	pantheon::Handle *Hand = Proc->GetHandle(Handle);
	if (Hand == nullptr)
	{
		Proc->Unlock();
		CurThread->Unlock();
		return pantheon::Result::SYS_FAIL;
	}
	
	/* Is this a handle to a process, or to a thread? */
	if (Hand->GetType() == pantheon::HANDLE_TYPE_PROCESS)
	{
		pantheon::Process *Other = Hand->GetContent().Process;
		Other->Lock();
		Other->SetState(pantheon::Process::STATE_RUNNING);
		Other->Unlock();

		Proc->Unlock();
		CurThread->Unlock();
		return pantheon::Result::SYS_OK;
	}
	else if (Hand->GetType() == pantheon::HANDLE_TYPE_THREAD)
	{
		pantheon::Thread *Other = Hand->GetContent().Thread;
		Other->Lock();
		Other->SetState(pantheon::Thread::STATE_WAITING);
		Other->Unlock();

		Proc->Unlock();
		CurThread->Unlock();
		return pantheon::Result::SYS_OK;
	}

	Proc->Unlock();
	CurThread->Unlock();
	return pantheon::Result::SYS_FAIL;	
}

pantheon::Result pantheon::SVCCreatePort(pantheon::TrapFrame *CurFrame)
{
	/* svc_CreatePort(PortName Name, INT64 SessionMax, INT32 *SrvHandle, INT32 *CliHandle) */
	CHAR Buffer[pantheon::ipc::PortNameLength] = {0};
	const CHAR *Location = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	CopyString(Buffer, Location, pantheon::ipc::PortNameLength);

	pantheon::ipc::PortName Name;
	CopyMemory((VOID*)&Name.AsNumber, (VOID*)Buffer, sizeof(pantheon::ipc::PortNameLength));

	INT64 SessionMax = (INT64)CurFrame->GetIntArgument(1);

	pantheon::ipc::Port *NewPort = pantheon::ipc::Port::Create();
	if (NewPort == nullptr)
	{
		/* No such port is possible to create: out of resources. */
		return pantheon::Result::SYS_OOM;
	}

	NewPort->Initialize(Name, SessionMax);

	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	pantheon::ScopedLock _PL(CurProc);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::ScopedLock _TL(CurThread);

	pantheon::Handle ServHandle = pantheon::Handle(NewPort);
	INT32 IndexS = CurProc->EncodeHandle(ServHandle);

	pantheon::Handle ClientHandle = pantheon::Handle(NewPort->GetClientPort());
	INT32 IndexC = CurProc->EncodeHandle(ClientHandle);

	INT32 *SrvPtr = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(2));
	INT32 *CliPtr = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(3));

	*SrvPtr = IndexS;
	*CliPtr = IndexC;

	return pantheon::Result::SYS_OK;	
}

typedef pantheon::Result (*SyscallFn)(pantheon::TrapFrame *);

SyscallFn syscall_table[] = 
{
	(SyscallFn)pantheon::SVCExitProcess, 
	(SyscallFn)pantheon::SVCForkProcess, 
	(SyscallFn)pantheon::SVCLogText, 
	(SyscallFn)pantheon::SVCAllocateBuffer,
	(SyscallFn)pantheon::SVCCreateThread,
	(SyscallFn)pantheon::SVCCreateNamedEvent,
	(SyscallFn)pantheon::SVCSignalEvent,
	(SyscallFn)pantheon::SVCClearEvent,
	(SyscallFn)pantheon::SVCResetEvent,
	(SyscallFn)pantheon::SVCPollEvent,
	(SyscallFn)pantheon::SVCYield,
	(SyscallFn)pantheon::SVCExitThread,
	(SyscallFn)pantheon::SVCExecute,
	(SyscallFn)pantheon::SVCCreatePort,
};

UINT64 pantheon::SyscallCount()
{
	constexpr UINT64 Count = sizeof(syscall_table) / (sizeof(void*));
	return Count;
}

BOOL pantheon::CallSyscall(UINT32 Index, pantheon::TrapFrame *Frame)
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return FALSE;
	}	
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return FALSE;
	}

	if (Index < SyscallCount() && Frame != nullptr)
	{
		pantheon::CPU::STI();
		pantheon::Result Result = (*syscall_table[Index])(Frame);
		Frame->Regs[0] = (UINT64)Result;
		pantheon::CPU::CLI();
		return TRUE;
	}
	return FALSE;
}