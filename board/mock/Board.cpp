#include <arch.hpp>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

void WriteSerialChar(CHAR Char)
{
}

void WriteString(const CHAR *String)
{

}

void BoardInit()
{

}

void _putchar(char c)
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