#include <kern_datatypes.hpp>
#include <Proc/kern_cpu.hpp>

#include "Syscalls.hpp"

extern "C" 
void enable_interrupts()
{
	pantheon::CPU::STI();
}

extern "C" 
void disable_interrupts()
{
	pantheon::CPU::CLI();
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

pantheon::Result pantheon::SVCLogText(const CHAR *Data)
{
	pantheon::CPU::GetCurThread()->Lock();
	SERIAL_LOG("%s\n", Data);
	pantheon::CPU::GetCurThread()->Unlock();
	return 0;
}

pantheon::Result pantheon::SVCAllocateBuffer(UINT64 Sz)
{
	/* HACK: sizeof(pantheon::Result) == sizeof(UINT_PTR)
	 * Until we get a more proper virtual memory API going,
	 * this will suffice. Not ideal, but better than nothing...
	 */
	return (UINT64)(BasicMalloc(Sz)());
}

/**
 * \~english @brief Creates a new thread for the given process.
 * \~english @param Entry The entry point for the new thread
 * \~english @param RESERVED Reserved space for future usage: will be for thread-local data. This space is reserved to ensure ABI compatibility in the future.
 * \~english @param StackTop The top of the stack for the newly made process
 * \~english @param Priority The priority of the new thread
 */
pantheon::Result pantheon::SVCCreateThread(
	ThreadStartPtr Entry, VOID *RESERVED, 
	void *StackTop, pantheon::ThreadPriority Priority)
{
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
pantheon::Result pantheon::SVCCreateNamedEvent(const CHAR *Name, UINT32 *ReadHandle, UINT32 *WriteHandle)
{
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

pantheon::Result pantheon::SVCSignalEvent(UINT32 WriteHandle)
{
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

pantheon::Result pantheon::SVCClearEvent(UINT32 WriteHandle)
{
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

pantheon::Result pantheon::SVCResetEvent(UINT32 ReadHandle)
{
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

pantheon::Result pantheon::SVCPollEvent(UINT32 Handle)
{
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

pantheon::Result pantheon::SVCExecute(UINT32 Handle)
{
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