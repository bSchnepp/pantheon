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

void *syscall_table[] = 
{
	(void*)pantheon::SVCExitProcess, 
	(void*)pantheon::SVCForkProcess, 
	(void*)pantheon::SVCLogText, 
	(void*)pantheon::SVCAllocateBuffer,
	(void*)pantheon::SVCCreateThread,
};