#include <stdint.h>

#include <kern.h>
#include <byte_swap.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>
#include <kern_runtime.hpp>

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

}
#endif