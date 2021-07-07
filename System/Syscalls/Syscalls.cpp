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
	pantheon::Thread *CurThread = pantheon::CPU::GetCoreInfo()->CurThread;
	CurThread->MyProc()->DeactivateThread(CurThread);
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
	pantheon::CPU::CLI();
	SERIAL_LOG("%s\n", Data);
	pantheon::CPU::STI();
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

void *syscall_table[] = 
{
	(void*)pantheon::SVCExitProcess, 
	(void*)pantheon::SVCForkProcess, 
	(void*)pantheon::SVCLogText, 
	(void*)pantheon::SVCAllocateBuffer
};