#include <kern_datatypes.hpp>

#ifndef _KERN_ALLOC_HPP_
#define _KERN_ALLOC_HPP_

namespace pantheon::PageAllocator
{
	UINT64 Alloc();
	void Free(UINT64 Page);
}

#endif