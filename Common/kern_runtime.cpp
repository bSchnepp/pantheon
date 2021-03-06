#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Sync/kern_spinlock.hpp>

#include <printf/printf.h>

VOID WriteMMIOU64(UINT64 Addr, UINT64 Value)
{
	*(volatile UINT64*)Addr = Value;
}

VOID WriteMMIOU32(UINT64 Addr, UINT32 Value)
{
	*(volatile UINT32*)Addr = Value;
}

VOID WriteMMIOU16(UINT64 Addr, UINT16 Value)
{
	*(volatile UINT16*)Addr = Value;
}

VOID WriteMMIOU8(UINT64 Addr, UINT8 Value)
{
	*(volatile UINT8*)Addr = Value;
}

VOID WriteMMIOS64(UINT64 Addr, INT64 Value)
{
	*(volatile INT64*)Addr = Value;
}

VOID WriteMMIOS32(UINT64 Addr, INT32 Value)
{
	*(volatile INT32*)Addr = Value;
}

VOID WriteMMIOS16(UINT64 Addr, INT16 Value)
{
	*(volatile INT16*)Addr = Value;
}

VOID WriteMMIOS8(UINT64 Addr, INT8 Value)
{
	*(volatile INT8*)Addr = Value;
}


UINT64 ReadMMIOU64(UINT64 Addr)
{
	return *(volatile UINT64*)Addr;
}

UINT32 ReadMMIOU32(UINT64 Addr)
{
	return *(volatile UINT32*)Addr;
}

UINT16 ReadMMIOU16(UINT64 Addr)
{
	return *(volatile UINT16*)Addr;
}

UINT8  ReadMMIOU8(UINT64 Addr)
{
	return *(volatile UINT8*)Addr;
}

INT64 ReadMMIOS64(UINT64 Addr)
{
	return *(volatile INT64*)Addr;
}

INT32 ReadMMIOS32(UINT64 Addr)
{
	return *(volatile INT32*)Addr;
}

INT16 ReadMMIOS16(UINT64 Addr)
{
	return *(volatile INT16*)Addr;
}

INT8  ReadMMIOS8(UINT64 Addr)
{
	return *(volatile INT8*)Addr;
}

BOOL StringCompare(const CHAR *Arg1, const CHAR *Arg2, UINT64 Amt)
{
	for (UINT64 Index = 0; Index < Amt; ++Index)
	{
		if (Arg1[Index] != Arg2[Index])
		{
			return FALSE;
		}

		if (Arg1[Index] == 0)
		{
			break;
		}
	}
	return TRUE;
}

void ClearBuffer(CHAR *Location, UINT32 Amount)
{
	UINT32 Index = 0;

	UINT64 *AsUINT64 = (UINT64*)Location;
	for (Index = 0; Index < Amount; Index += 8)
	{
		AsUINT64[Index / 8] = 0;
	}

	for (; Index < Amount; ++Index)
	{
		Location[Index] = '\0';
	}
}

void CopyMemory(VOID *Dest, VOID *Src, UINT64 Amt)
{
	CHAR *DestAsChar = reinterpret_cast<CHAR*>(Dest);
	CHAR *SrcAsChar = reinterpret_cast<CHAR*>(Src);
	for (UINT64 Index = 0; Index < Amt; ++Index)
	{
		DestAsChar[Index] = SrcAsChar[Index];
	}
}

void SERIAL_LOG_UNSAFE(const char *Fmt, ...)
{
	va_list Args;
	va_start(Args, Fmt);
	vprintf(Fmt, Args);
	va_end(Args);	
}

/* Note that until some of the issues with static constructors are resolved,
 * this mutex is technically in undefined state. We'll need .init_array to work
 * right first...
 */
static pantheon::Spinlock PrintMutex;

void SERIAL_LOG(const char *Fmt, ...)
{
	PrintMutex.Acquire();
	va_list Args;

	va_start(Args, Fmt);
	vprintf(Fmt, Args);
	va_end(Args);
	PrintMutex.Release();
}