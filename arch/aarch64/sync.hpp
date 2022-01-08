#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#ifndef _ARCH_SYNC_HPP_
#define _ARCH_SYNC_HPP_

namespace pantheon::Sync
{
	FORCE_INLINE void DSBISH()
	{
		asm volatile("dsb ish\n" ::: "memory");
	}

	FORCE_INLINE void DSBSY()
	{
		asm volatile("dsb sy\n" ::: "memory");
	}

	FORCE_INLINE void ISB()
	{
		asm volatile("isb\n" ::: "memory");
	}
}

#endif