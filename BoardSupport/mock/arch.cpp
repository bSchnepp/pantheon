#include <kern.h>

#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#include <Proc/kern_cpu.hpp>
#include <System/Syscalls/Syscalls.hpp>

void createprocess_tail()
{

}

void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset)
{
	
}

UINT8 pantheon::CPU::GetProcessorNumber()
{
	return 0;
}

VOID pantheon::CPU::CLI()
{
}

VOID pantheon::CPU::STI()
{
}

VOID pantheon::CPU::PAUSE()
{

}

VOID pantheon::RearmSystemTimer()
{

}

VOID pantheon::RearmSystemTimer(UINT64 Freq)
{

}

VOID pantheon::DisableSystemTimer()
{

}

extern "C" INT32 CallSMC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" INT32 CallHVC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" VOID asm_kern_init_core()
{

}

extern "C" VOID drop_usermode(UINT64 PC)
{
	PANTHEON_UNUSED(PC);
}

extern "C" UINT64 svc_LogText(const CHAR *Text)
{
	return pantheon::SVCLogText(Text);
}

#include <stdlib.h>

extern "C" void *svc_AllocateBuffer(UINT64 Sz)
{
	return malloc(Sz);
}

extern "C" void svc_CreateThread(void *Entry, VOID *Reserved, void *StackTop, pantheon::ThreadPriority Priority)
{
	PANTHEON_UNUSED(Entry);
	PANTHEON_UNUSED(Reserved);
	PANTHEON_UNUSED(StackTop);
	PANTHEON_UNUSED(Priority);
}

VOID PerCoreBoardInit()
{
	
}

VOID pantheon::CPU::PUSHI()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	CoreInfo->NOff++;
}

VOID pantheon::CPU::POPI()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	CoreInfo->NOff--;
}

VOID pantheon::CPU::HLT()
{
	return;
}

UINT64 pantheon::CPU::ICOUNT()
{
	pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
	return CoreInfo->NOff;	
}