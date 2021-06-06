#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>


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

VOID pantheon::RearmSystemTimer()
{

}

VOID pantheon::RearmSystemTimer(UINT64 Freq)
{

}

VOID pantheon::DisableSystemTimer()
{

}

extern "C" INT64 CallSMC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" INT64 CallHVC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" VOID asm_kern_init_core()
{

}

VOID PerCoreBoardInit()
{
	
}