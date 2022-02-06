#include <stdint.h>

#include <kern.h>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>
#include <kern_runtime.hpp>

/* Necessary since when portions of the kernel are recompiled in userspace
 * to run on the host (ie, for CI/CD), we don't want to break the libc
 * implementations.
 */
#ifndef ONLY_TESTS

void *operator new(UINT64 Sz)
{
	PANTHEON_UNUSED(Sz);
	return BasicMalloc(Sz)();
}

void operator delete(void *Ptr)
{
	PANTHEON_UNUSED(Ptr);
}

extern "C"
{

void __cxa_pure_virtual()
{
	/* Do nothing. Possibly warn? */
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

/* TODO: Find out if we need to properly use these in any way... */
void __cxa_guard_acquire()
{

}

void __cxa_guard_release()
{
	
}

extern "C" void *memcpy(void *dest, const void * src, unsigned long int n)
{
	char *AsCharDst = (char*)(dest);
	const char *AsCharSrc = (const char*)(src);
	for (unsigned int Index = 0; Index < n; ++Index)
	{
		AsCharDst[Index] = AsCharSrc[Index];
	}
	return AsCharDst;
}

extern "C" void *memset(void *dest, int v, unsigned long int n)
{
	unsigned char *AsCharDst = (unsigned char*)(dest);
	for (unsigned int Index = 0; Index < n; ++Index)
	{
		AsCharDst[Index] = (unsigned char)v;
	}
	return AsCharDst;
}

/* Assume we're ever only going to run on 64-bit systems. 
 * Aarch32 (A-class) is getting rarer, and support has been dropped in kernel
 * mode since the A76 anyway.
 */

uintptr_t __stack_chk_guard = 0xDEADC0DEDEADBEEF;

[[noreturn]] VOID __stack_chk_fail(void)
{
	pantheon::StopErrorFmt("Stop error: stack canary was smashed\n");
}

}
#endif


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
	uint16_t UnsignedItem = Item;
	uint16_t Swap = __builtin_bswap16(UnsignedItem);
	return (int16_t)(Swap);
}

int32_t SwapBytes(int32_t Item)
{
	uint32_t UnsignedItem = Item;
	uint32_t Swap = __builtin_bswap32(UnsignedItem);
	return (int32_t)(Swap);
}

int64_t SwapBytes(int64_t Item)
{
	uint64_t UnsignedItem = Item;
	uint64_t Swap = __builtin_bswap64(UnsignedItem);
	return (int64_t)(Swap);
}

