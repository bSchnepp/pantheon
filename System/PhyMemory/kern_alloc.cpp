#include <Boot/Boot.hpp>
#include <Sync/kern_spinlock.hpp>
#include <System/PhyMemory/kern_alloc.hpp>

static pantheon::Spinlock AllocLock;

void pantheon::PageAllocator::InitPageAllocator()
{
	AllocLock = pantheon::Spinlock("page_alloc");
}

UINT64 pantheon::PageAllocator::Alloc()
{
	AllocLock.Acquire();
	UINT64 Addr = FindPage();
	AllocatePage(Addr);
	AllocLock.Release();

	return Addr;
}

void pantheon::PageAllocator::Free(UINT64 Page)
{
	AllocLock.Acquire();
	FreePage(Page);
	AllocLock.Release();
}