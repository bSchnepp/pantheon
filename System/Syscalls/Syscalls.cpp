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
	pantheon::GetGlobalScheduler()->ReleaseThread(CurThread);
	CurThread->MyProc()->SetState(pantheon::PROCESS_STATE_ZOMBIE);
	pantheon::CPU::GetCoreInfo()->CurSched->Reschedule();
}

pantheon::Result pantheon::SVCForkProcess()
{
	/* nyi */
	return 0;
}

pantheon::Result pantheon::SVCLogText(const CHAR *Data)
{
	pantheon::CPU::PUSHI();
	SERIAL_LOG("%s\n", Data);
	pantheon::CPU::POPI();
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
	return pantheon::GetGlobalScheduler()->CreateThread(Proc, (void*)Entry, RESERVED, Priority, StackTop);
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
pantheon::Result pantheon::SVCCreateNamedEvent(const CHAR *Name, UINT8 *ReadHandle, UINT8 *WriteHandle)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
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

pantheon::Result pantheon::SVCSignalEvent(UINT8 WriteHandle)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
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

pantheon::Result pantheon::SVCClearEvent(UINT8 WriteHandle)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();

	pantheon::Handle *Hand = Proc->GetHandle(WriteHandle);
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

pantheon::Result pantheon::SVCResetEvent(UINT8 ReadHandle)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::Handle *Hand = Proc->GetHandle(ReadHandle);
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

pantheon::Result pantheon::SVCPollEvent(UINT8 Handle)
{
	pantheon::Process *Proc = pantheon::CPU::GetCurThread()->MyProc();
	pantheon::Handle *Hand = Proc->GetHandle(Handle);
	if (Hand->GetType() != pantheon::HANDLE_TYPE_READ_SIGNAL)
	{
		return -1;
	}

	pantheon::ipc::ReadableEvent *Evt = Hand->GetContent().ReadEvent;
	if (Evt)
	{
		return Evt->Parent->Status == pantheon::ipc::EVENT_TYPE_SIGNALED;
	}
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
};