#include <stdint.h>

#include <kern.h>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

/* Necessary since when portions of the kernel are recompiled in userspace
 * to run on the host (ie, for CI/CD), we don't want to break the libc
 * implementations.
 */
#ifndef ONLY_TESTS
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

