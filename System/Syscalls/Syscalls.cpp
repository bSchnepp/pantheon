#include <kern_datatypes.hpp>
#include <Proc/kern_cpu.hpp>

#include "Syscalls.hpp"
#include <vmm/vmm.hpp>

#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

#include <IPC/kern_port.hpp>
#include <IPC/kern_event.hpp>

#include <IPC/kern_server_port.hpp>
#include <IPC/kern_server_connection.hpp>

#include <IPC/kern_client_port.hpp>
#include <IPC/kern_client_connection.hpp>

template<typename T>
static T ReadArgument(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return (T)Val;
}


static const CHAR *ReadArgumentAsCharPointer(UINT64 Val)
{
	/* Don't allow reading of kernel memory */
	if (Val > pantheon::vmm::HigherHalfAddress)
	{
		return nullptr;
	}

	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	if (CurProc->IsLocked() == FALSE)
	{
		pantheon::StopErrorFmt("Attempt to read userspace memory without CurProc lock (PID %hhu)\n", CurProc->ProcessID());
	}

	/* Make sure we translate this to something in kernel memory, 
	 * just to be safe. */
	pantheon::vmm::VirtualAddress RawPtr = Val;
	const CHAR *Result = nullptr;

	pantheon::vmm::PhysicalAddress PAddr = pantheon::vmm::VirtualToPhysicalAddress(CurProc->GetPageTable(), RawPtr);
	pantheon::vmm::VirtualAddress VAddr = pantheon::vmm::PhysicalToVirtualAddress(PAddr);

	Result = reinterpret_cast<const char *>(VAddr);

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

	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	if (CurProc->IsLocked() == FALSE)
	{
		pantheon::StopErrorFmt("Attempt to read userspace memory without CurProc lock (PID %hhu)\n", CurProc->ProcessID());
	}

	/* Make sure we translate this to something in kernel memory, 
	 * just to be safe. */
	pantheon::vmm::VirtualAddress RawPtr = Val;
	T *Result = nullptr;

	pantheon::vmm::PhysicalAddress PAddr = pantheon::vmm::VirtualToPhysicalAddress(CurProc->GetPageTable(), RawPtr);
	pantheon::vmm::VirtualAddress VAddr = pantheon::vmm::PhysicalToVirtualAddress(PAddr);

	Result = reinterpret_cast<T*>(VAddr);

	return Result;	


}

static UINT64 ReadArgumentAsInteger(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<UINT64>(Val);
}

VOID pantheon::SVCExitProcess(pantheon::TrapFrame *CurFrame)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	PANTHEON_UNUSED(CurFrame);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	CurThread->MyProc()->SetState(pantheon::Process::STATE_ZOMBIE);

	CurThread->Lock();
	CurThread->SetState(pantheon::Thread::STATE_TERMINATED);
	CurThread->SetState(pantheon::Thread::STATE_DEAD);
	CurThread->Unlock();
	
	pantheon::CPU::GetCoreInfo()->CurSched->Reschedule();
}

pantheon::Result pantheon::SVCForkProcess(pantheon::TrapFrame *CurFrame)
{
	/* nyi */
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	PANTHEON_UNUSED(CurFrame);
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCLogText(pantheon::TrapFrame *CurFrame)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock _P(Proc);

	const CHAR *Data = nullptr;
	
	Data = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	if (Data == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	SERIAL_LOG("%s\n", Data);
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
	pantheon::ScopedGlobalSchedulerLock _SL;

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

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
	pantheon::ScopedGlobalSchedulerLock _SL;

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	INT32 WriteHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));

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
	pantheon::ScopedGlobalSchedulerLock _SL;

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	INT32 WriteHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));

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
	pantheon::ScopedGlobalSchedulerLock _SL;

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	INT32 ReadHandle = static_cast<INT32>(CurFrame->GetIntArgument(0));

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
	pantheon::ScopedGlobalSchedulerLock _SL;

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::ScopedLock ScopeLockProc(Proc);

	INT32 Handle = static_cast<INT32>(CurFrame->GetIntArgument(0));

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
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	pantheon::ScopedLock _PL(CurProc);

	CHAR Buffer[pantheon::ipc::PortNameLength] = {0};
	const CHAR *Location = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	CopyString(Buffer, Location, pantheon::ipc::PortNameLength);

	pantheon::ipc::PortName Name;
	CopyMemory((VOID*)&Name.AsNumber, (VOID*)Buffer, sizeof(pantheon::ipc::PortNameLength));

	INT64 SessionMax = (INT64)CurFrame->GetIntArgument(1);

	INT32 *SrvPtr = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(2));
	INT32 *CliPtr = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(3));

	pantheon::ipc::Port *NewPort = pantheon::ipc::Port::Create();
	if (NewPort == nullptr)
	{
		/* No such port is possible to create: out of resources. */
		return pantheon::Result::SYS_OOM;
	}

	NewPort->Initialize(Name, SessionMax);

	pantheon::Handle ServHandle = pantheon::Handle(NewPort->GetServerPort());
	INT32 IndexS = CurProc->EncodeHandle(ServHandle);

	pantheon::Handle ClientHandle = pantheon::Handle(NewPort->GetClientPort());
	INT32 IndexC = CurProc->EncodeHandle(ClientHandle);

	*SrvPtr = IndexS;
	*CliPtr = IndexC;

	return pantheon::Result::SYS_OK;	
}

pantheon::Result pantheon::SVCConnectToPort(pantheon::TrapFrame *CurFrame)
{
	/* NYI */
	PANTHEON_UNUSED(CurFrame);
	return pantheon::Result::SYS_FAIL;
}

/**
 * @brief Establish a Connection to a named Port
 * @details A connection is created with a named Port, which is globally available and does not enforce any permission checks.
 * Ordinary processes generally cannot create named Ports, so this system call is typically used for connecting to Ports
 * which provide essential runtime services, such as sysm.
 */
pantheon::Result pantheon::SVCConnectToNamedPort(pantheon::TrapFrame *CurFrame)
{
	/* svc_ConnectToNamedPort(const char *Name, INT32 *OutConn) */
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	pantheon::ScopedLock _L(CurProc);

	const char *Name = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	if (Name == nullptr)
	{
		return pantheon::Result::SYS_FAIL;	
	}

	pantheon::ipc::PortName PName;
	CopyString(PName.AsChars, Name, pantheon::ipc::PortNameLength);

	pantheon::ipc::Port *NamedPort = pantheon::ipc::Port::GetRegistered(PName);
	if (NamedPort == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	INT32 *HandleLocation = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(1));
	if (HandleLocation == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::ClientPort *CliPort = NamedPort->GetClientPort();

	/* If, for *whatever* reason this is null (ie, client port is closed), return an error.
	 * These results should probably be more descriptive in the future, but for now this is OK. 
	 */
	if (CliPort == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	/* TODO: Enforce resource limits, or some kind of reference counting.
	 * This will OOM very fast!!!
	 */
	pantheon::ipc::ClientConnection *NewConn = NamedPort->CreateConnection();
	if (NewConn == nullptr)
	{
		/* We might have run out of connections... */
		return pantheon::Result::SYS_OOM;	
	}

	*HandleLocation = CurProc->EncodeHandle(pantheon::Handle(NewConn));

	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCAcceptConnection(pantheon::TrapFrame *CurFrame)
{
	/* svc_AcceptConnection(INT32 InServerPortHandle, INT32 *OutConnection); */
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	pantheon::ScopedLock _L(CurProc);

	INT32 InHandle = CurFrame->GetRawArgument<INT32>(0);
	INT32 *OutHandle = ReadArgumentAsPointer<INT32>(CurFrame->GetIntArgument(1));

	pantheon::Handle *Hand = CurProc->GetHandle(InHandle);
	if (!Hand)
	{
		*OutHandle = -1;
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::ServerPort *Port = Hand->GetPtr<pantheon::ipc::ServerPort>();
	if (!Port)
	{
		*OutHandle = -1;
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::ipc::ServerConnection *Conn = Port->Dequeue();
	if (!Conn)
	{
		*OutHandle = -1;
		return pantheon::Result::SYS_FAIL;
	}

	INT32 Res = CurProc->EncodeHandle(pantheon::Handle(Conn));
	if (Res >= 0)
	{
		*OutHandle = Res;
		return pantheon::Result::SYS_OK;
	}
	return pantheon::Result::SYS_FAIL;
}

pantheon::Result pantheon::SVCReplyAndRecieve(pantheon::TrapFrame *CurFrame)
{
	/* (INT32 *ServerHandle, UINT16 NumHandles, INT32 *Handles) */
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	{
		pantheon::ScopedLock _L(CurProc);

		INT32 *ServerHandle = ReadArgumentAsPointer<INT32>(CurFrame->Regs[0]);

		/* If this handle is invalid, return FAIL. */
		INT32 CurHandle = *ServerHandle;
		if (CurHandle < 0)
		{
			*ServerHandle = pantheon::INVALID_HANDLE_ID;
			return pantheon::Result::SYS_FAIL;
		}

		pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
		pantheon::ScopedLock _TL(CurThread);


		/* Grab the current connection from the port */
		pantheon::Handle *Hand = CurProc->GetHandle(CurHandle);
		if (Hand->GetType() != pantheon::HANDLE_TYPE_SERVER_CONNECTION)
		{
			/* This isn't what we were expecting... */
			return pantheon::Result::SYS_FAIL;
		}

		/* Go grab the connection we expected to go through... */
		pantheon::ipc::ServerConnection *Conn = CurProc->GetHandle(CurHandle)->GetPtr<pantheon::ipc::ServerConnection>();
		if (Conn == nullptr)
		{
			return pantheon::Result::SYS_FAIL;
		}

		/* Grab the message through the connection: our reply will already be in the TLS. */
		pantheon::Thread::ThreadLocalRegion *Region = CurThread->GetThreadLocalArea();
		pantheon::Result Res = Conn->IssueReply(Region->RawData);
		if (Res != pantheon::Result::SYS_OK)
		{
			return Res;
		}

		CurThread->SetState(pantheon::Thread::STATE_BLOCKED);
	}

	/* NYI: Wait for subsequent responses */





	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCCloseHandle(pantheon::TrapFrame *CurFrame)
{
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();
	pantheon::ScopedLock _L(CurProc);

	INT32 Handle = CurFrame->GetRawArgument<INT32>(0);

	if (Handle < 0)
	{
		return pantheon::Result::SYS_FAIL;
	}

	pantheon::Handle *Hand = CurProc->GetHandle(Handle);
	if (Hand == nullptr)
	{
		return pantheon::Result::SYS_FAIL;
	}

	Hand->Close();
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::SVCSendRequest(pantheon::TrapFrame *CurFrame)
{
	/* NYI */
	PANTHEON_UNUSED(CurFrame);
	return pantheon::Result::SYS_FAIL;
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
	(SyscallFn)pantheon::SVCConnectToPort,
	(SyscallFn)pantheon::SVCConnectToNamedPort,
	(SyscallFn)pantheon::SVCAcceptConnection,
	(SyscallFn)pantheon::SVCReplyAndRecieve,
	(SyscallFn)pantheon::SVCCloseHandle,
	(SyscallFn)pantheon::SVCSendRequest,
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