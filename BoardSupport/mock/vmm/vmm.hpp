/* Dont do anything */
#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <arch.hpp>
#include <sync.hpp>
#include <vmm/pte.hpp>
#include <Common/Structures/kern_slab.hpp>

#include <Boot/Boot.hpp>


#ifndef VMM_HPP_
#define VMM_HPP_

namespace pantheon::vmm
{


FORCE_INLINE VirtualAddress PhysicalToVirtualAddress(PhysicalAddress PhyAddr)
{
	return PhyAddr;
}

static constexpr FORCE_INLINE UINT16 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
{
	/* Only valid for 0, 1, 2, or 3. */
	Level = (Level > 3) ? 3 : Level;
	UINT64 Shift = 39 - (Level * 9);
	return (VAddr >> Shift) & 0x1FF;
}

FORCE_INLINE PhysicalAddress VirtualToPhysicalAddress(PageTable *RootTable, VirtualAddress VirtAddr)
{
	return VirtAddr;
}


class PageAllocator
{
public:
	FORCE_INLINE PageAllocator() : PageAllocator(nullptr, 0) {}

	FORCE_INLINE PageAllocator(VOID *Area, UINT64 Pages)
	{
		this->Allocator = pantheon::mm::SlabCache<pantheon::vmm::PageTable>(Area, Pages);
	}
	
	FORCE_INLINE ~PageAllocator()
	{

	}

	FORCE_INLINE pantheon::vmm::PageTable *Allocate()
	{
		pantheon::vmm::PageTable *NewTable = this->Allocator.AllocateNoCtor();
		ClearBuffer((char*)NewTable, sizeof(pantheon::vmm::PageTable));
		return NewTable;
	}

	[[nodiscard]] FORCE_INLINE UINT64 SpaceLeft() const
	{
		return this->Allocator.SpaceLeft();
	}

	[[nodiscard]] FORCE_INLINE UINT64 SpaceUsed() const
	{
		return this->Allocator.SlabCount() - this->SpaceLeft();
	}

	BOOL Map(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		return TRUE;
	}

	BOOL Reprotect(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		return TRUE;
	}

	FORCE_INLINE BOOL MapLower(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		return TRUE;
	}

	FORCE_INLINE BOOL ReprotectLower(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		return TRUE;
	}

private:
	static FORCE_INLINE void PanicOnNullPageTable(pantheon::vmm::PageTable *Table)
	{
	}

	static FORCE_INLINE void VMMSync()
	{
	}

	static FORCE_INLINE UINT16 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
	{
		/* Only valid for 0, 1, 2, or 3. */
		Level = (Level > 3) ? 3 : Level;
		UINT64 Shift = 39 - (Level * 9);
		return (VAddr >> Shift) & 0x1FF;
	}

	FORCE_INLINE void CreateBlock(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
	{
	}

	FORCE_INLINE void CreateTable(pantheon::vmm::PageTableEntry *Entry)
	{
	}

	FORCE_INLINE void CreateBlockLower(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
	{
	}

	FORCE_INLINE void CreateTableLower(pantheon::vmm::PageTableEntry *Entry)
	{
	}

	FORCE_INLINE BOOL CreateGreedyBlock(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
		return TRUE;
	}

	FORCE_INLINE BOOL CreateGreedyBlockLower(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
		return TRUE;
	}

	pantheon::mm::SlabCache<pantheon::vmm::PageTable> Allocator;
};

}

#endif