#include <kern_datatypes.hpp>
#include <Proc/kern_cpu.hpp>

#include "Syscalls.hpp"

template<typename T>
static T ReadArgument(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<T>(Val);
}


static const CHAR *ReadArgumentAsCharPointer(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<const CHAR*>(Val);
}

static UINT64 ReadArgumentAsInteger(UINT64 Val)
{
	/* This is a hack: this should be validated later!!! */
	return reinterpret_cast<UINT64>(Val);
}

VOID pantheon::SVCExitProcess()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	CurThread->Lock();
	pantheon::GetGlobalScheduler()->ReleaseThread(CurThread);
	CurThread->MyProc()->SetState(pantheon::PROCESS_STATE_ZOMBIE);
	CurThread->Unlock();
	pantheon::CPU::GetCoreInfo()->CurSched->Reschedule();
}

pantheon::Result pantheon::SVCForkProcess()
{
	/* nyi */
	return 0;
}

pantheon::Result pantheon::SVCLogText()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	const CHAR *Data = nullptr;
	
	Data = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	if (Data == nullptr)
	{
		return -1;
	}

	pantheon::CPU::GetCurThread()->Lock();
	SERIAL_LOG("%s\n", Data);
	pantheon::CPU::GetCurThread()->Unlock();
	return 0;
}

pantheon::Result pantheon::SVCAllocateBuffer()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT64 Sz = 0;
	
	Sz = ReadArgumentAsInteger(CurFrame->GetIntArgument(0));

	/* HACK: sizeof(pantheon::Result) == sizeof(UINT_PTR)
	 * Until we get a more proper virtual memory API going,
	 * this will suffice. Not ideal, but better than nothing...
	 */
	return (UINT64)(BasicMalloc(Sz)());
}

typedef void (*ThreadStartPtr)(void*);

/**
 * \~english @brief Creates a new thread for the given process.
 * \~english @param Entry The entry point for the new thread
 * \~english @param RESERVED Reserved space for future usage: will be for thread-local data. This space is reserved to ensure ABI compatibility in the future.
 * \~english @param StackTop The top of the stack for the newly made process
 * \~english @param Priority The priority of the new thread
 */
pantheon::Result pantheon::SVCCreateThread()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	ThreadStartPtr Entry = nullptr;
	Entry = ReadArgument<ThreadStartPtr>(CurFrame->GetIntArgument(0));

	VOID *RESERVED = nullptr;
	RESERVED = ReadArgument<VOID*>(CurFrame->GetIntArgument(1));

	VOID *StackTop = nullptr;
	StackTop = ReadArgument<VOID*>(CurFrame->GetIntArgument(2));

	pantheon::ThreadPriority Priority = pantheon::THREAD_PRIORITY_NORMAL;
	Priority = (pantheon::ThreadPriority)CurFrame->GetIntArgument(3);

	if (Entry == nullptr || StackTop == nullptr)
	{
		return -1;
	}

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}

	pantheon::ScopedLock SL(Proc);
	pantheon::Thread *Thr = pantheon::GetGlobalScheduler()->CreateThread(Proc, (void*)Entry, RESERVED, Priority, StackTop);
	if (Thr == nullptr)
	{
		return -1;	
	}

	INT64 Result = Proc->EncodeHandle(pantheon::Handle(Thr));

	if (Result < 0)
	{
		return -1;
	}
	return (UINT32)Result;
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
pantheon::Result pantheon::SVCCreateNamedEvent()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	const CHAR *Name = nullptr;
	Name = ReadArgumentAsCharPointer(CurFrame->GetIntArgument(0));
	
	UINT32 *ReadHandle = nullptr;
	ReadHandle = ReadArgument<UINT32*>(CurFrame->GetIntArgument(1));

	UINT32 *WriteHandle = nullptr;
	WriteHandle = ReadArgument<UINT32*>(CurFrame->GetIntArgument(2));

	if (Name == nullptr || ReadHandle == nullptr || WriteHandle == nullptr)
	{
		return -1;
	}


	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	pantheon::ScopedLock ScopeLockProc(Proc);
	pantheon::String EvtName(Name);

	pantheon::ipc::NamedEvent *Evt = pantheon::ipc::LookupEvent(EvtName);
	if (Evt != nullptr)
	{
		UINT8 Write = Proc->EncodeHandle(pantheon::Handle(Evt->Writable));
		UINT8 Read = Proc->EncodeHandle(pantheon::Handle(Evt->Readable));

		*WriteHandle = Write;
		*ReadHandle = Read;
		return 0;
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
		return 0;
	}
	return -1;
}

pantheon::Result pantheon::SVCSignalEvent()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT32 WriteHandle = CurFrame->GetIntArgument(0);


	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
	if (Hand == nullptr)
	{
		return -1;
	}

	if (Hand->GetType() != pantheon::HANDLE_TYPE_WRITE_SIGNAL)
	{
		return -1;
	}

	pantheon::ipc::WritableEvent *Evt = Hand->GetContent().WriteEvent;
	
	if (Evt)
	{
		Evt->Signaler = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_SIGNALED;
	}

	/* TODO: wakeup every process waiting on it */
	return 0;
}

pantheon::Result pantheon::SVCClearEvent()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT32 WriteHandle = CurFrame->GetIntArgument(0);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
	if (Hand == nullptr)
	{
		return -1;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_WRITE_SIGNAL)
	{
		return -1;
	}

	pantheon::ipc::WritableEvent *Evt = Hand->GetContent().WriteEvent;

	if (Evt)
	{
		Evt->Parent->Readable->Clearer = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_UNSIGNALED;
	}
	return 0;
}

pantheon::Result pantheon::SVCResetEvent()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT32 ReadHandle = CurFrame->GetIntArgument(0);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(ReadHandle);
	if (Hand == nullptr)
	{
		return -1;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_READ_SIGNAL)
	{
		return -1;
	}

	pantheon::ipc::ReadableEvent *Evt = Hand->GetContent().ReadEvent;
	if (Evt)
	{
		Evt->Clearer = Proc;
		Evt->Parent->Status = pantheon::ipc::EVENT_TYPE_UNSIGNALED;
		return 0;
	}
	return -1;
}

pantheon::Result pantheon::SVCPollEvent()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT32 Handle = CurFrame->GetIntArgument(0);

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}
	pantheon::ScopedLock ScopeLockThread(CurThread);

	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	pantheon::ScopedLock ScopeLockProc(Proc);

	pantheon::Handle *Hand = Proc->GetHandle(Handle);
	if (Hand == nullptr)
	{
		return -1;
	}
	
	if (Hand->GetType() != pantheon::HANDLE_TYPE_READ_SIGNAL)
	{
		return -1;
	}

	pantheon::ipc::ReadableEvent *Evt = Hand->GetContent().ReadEvent;
	if (Evt)
	{
		BOOL Result = Evt->Parent->Status == pantheon::ipc::EVENT_TYPE_SIGNALED;
		return Result;
	}
	return -1;
}

pantheon::Result pantheon::SVCYield()
{
	pantheon::Scheduler *CurSched = pantheon::CPU::GetCurSched();
	{
		pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
		pantheon::ScopedLock ScopeLockThread(CurThread);
		CurThread->SetTicks(0);
	}
	CurSched->Reschedule();
	return 0;
}

pantheon::Result pantheon::SVCExitThread()
{
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Process *CurProc = CurThread->MyProc();
	pantheon::Scheduler *CurSched = pantheon::CPU::GetCurSched();
	CurProc->Lock();
	CurThread->Lock();
	CurThread->SetState(pantheon::THREAD_STATE_TERMINATED);
	CurThread->Unlock();

	if (pantheon::GetGlobalScheduler()->CountThreads(CurThread->MyProc()->ProcessID()) == 0)
	{
		CurProc->SetState(pantheon::PROCESS_STATE_ZOMBIE);
	}
	CurProc->Unlock();
	CurSched->Reschedule();
	return 0;
}

pantheon::Result pantheon::SVCExecute()
{
	pantheon::TrapFrame *CurFrame = pantheon::CPU::GetCurFrame();
	UINT32 Handle = CurFrame->GetIntArgument(0);
	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	if (!CurThread)
	{
		StopError("System call with no thread");
		return -1;
	}	
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	if (Proc == nullptr)
	{
		StopError("System call with no process");
		return -1;
	}
	CurThread->Lock();
	Proc->Lock();

	pantheon::Handle *Hand = Proc->GetHandle(Handle);
	if (Hand == nullptr)
	{
		Proc->Unlock();
		CurThread->Unlock();
		return -1;
	}
	
	/* Is this a handle to a process, or to a thread? */
	if (Hand->GetType() == pantheon::HANDLE_TYPE_PROCESS)
	{
		pantheon::Process *Other = Hand->GetContent().Process;
		Other->Lock();
		Other->SetState(pantheon::PROCESS_STATE_RUNNING);
		Other->Unlock();

		Proc->Unlock();
		CurThread->Unlock();
		return 0;
	}
	else if (Hand->GetType() == pantheon::HANDLE_TYPE_THREAD)
	{
		pantheon::Thread *Other = Hand->GetContent().Thread;
		Other->Lock();
		Other->SetState(pantheon::THREAD_STATE_WAITING);
		Other->Unlock();

		Proc->Unlock();
		CurThread->Unlock();
		return 0;
	}

	Proc->Unlock();
	CurThread->Unlock();
	return -1;	
}

void *syscall_table[] = 
{
	(void*)pantheon::SVCExitProcess, 
	(void*)pantheon::SVCForkProcess, 
	(void*)pantheon::SVCLogText, 
	(void*)pantheon::SVCAllocateBuffer,
	(void*)pantheon::SVCCreateThread,
	(void*)pantheon::SVCCreateNamedEvent,
	(void*)pantheon::SVCSignalEvent,
	(void*)pantheon::SVCClearEvent,
	(void*)pantheon::SVCResetEvent,
	(void*)pantheon::SVCPollEvent,
	(void*)pantheon::SVCYield,
	(void*)pantheon::SVCExitThread,
	(void*)pantheon::SVCExecute,
};