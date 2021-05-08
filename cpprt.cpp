#include <stdint.h>

#include <kern.h>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

extern "C"
{

[[noreturn]]
void __cxa_pure_virtual()
{
	/* Do nothing. Possibly warn? */
	for (;;){}
}

int __cxa_atexit(void (*f)(void*), void *obj, void *dso)
{
	/* ignore */
	PANTHEON_UNUSED(f);
	PANTHEON_UNUSED(obj);
	PANTHEON_UNUSED(dso);
	return 0;
}

void __cxa_finalize(void *f)
{
	/* Do nothing. */
	PANTHEON_UNUSED(f);
}

}

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

uint8_t SwapBytes(uint8_t Item)
{
	return Item;
}

uint16_t SwapBytes(uint16_t Item)
{
	return __builtin_bswap16(Item);
}

uint32_t SwapBytes(uint32_t Item)
{
	return __builtin_bswap32(Item);
}

uint64_t SwapBytes(uint64_t Item)
{
	return __builtin_bswap64(Item);
}

int8_t SwapBytes(int8_t Item)
{
	return Item;
}

int16_t SwapBytes(int16_t Item)
{
	return __builtin_bswap16(Item);
}

int32_t SwapBytes(int32_t Item)
{
	return __builtin_bswap32(Item);
}

int64_t SwapBytes(int64_t Item)
{
	return __builtin_bswap64(Item);
}

UINT64 StringCompare(void *L, void *R, UINT64 Amt)
{
	CHAR *CL = reinterpret_cast<CHAR*>(L);
	CHAR *CR = reinterpret_cast<CHAR*>(R);

	for (UINT64 Index = 0; Index < Amt; ++Index)
	{
		if (CL[Index] != CR[Index])
		{
			return FALSE;
		}
		if (CL[Index] == '\0')
		{
			break;
		}
	}
	return TRUE;
}