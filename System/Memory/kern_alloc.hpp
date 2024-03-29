#include <Boot/Boot.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_ALLOC_HPP_
#define _KERN_ALLOC_HPP_

namespace pantheon::PageAllocator
{
	void InitPageAllocator(InitialBootInfo *BootInfo);
	UINT64 Alloc();
	void Free(UINT64 Page);
	bool Used(UINT64 Addr);
}

#endif