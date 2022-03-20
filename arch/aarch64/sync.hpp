#include <kern_runtime.hpp>
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

	FORCE_INLINE void IC_IALLU()
	{
		asm volatile("ic iallu\n" ::: "memory");
	}

	FORCE_INLINE void FORCE_CLEAN_CACHE()
	{
		/* Be sure we're cleaning caches correctly */
		pantheon::Sync::ISB();
		pantheon::Sync::DSBSY();
		pantheon::Sync::IC_IALLU();
		pantheon::Sync::DSBSY();
		pantheon::Sync::ISB();
	}
}

#endif