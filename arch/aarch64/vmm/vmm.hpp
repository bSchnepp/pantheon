#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/sync.hpp>
#include <arch/aarch64/vmm/pte.hpp>
#include <Common/Structures/kern_slab.hpp>

#include <Boot/Boot.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

extern CHAR *VIRT_LOAD_ADDRESS;

namespace pantheon::vmm
{

static constexpr pantheon::vmm::VirtualAddress HigherHalfAddress = (1ULL << 63);

FORCE_INLINE VirtualAddress PhysicalToVirtualAddress(PhysicalAddress PhyAddr)
{
	/* Unlikely: only for before kernel loading */
	if (pantheon::CPUReg::R_TTBR1_EL1() == 0)
	{
		return PhyAddr;
	}
	return PhyAddr + PHYSICAL_MAP_AREA_ADDRESS; 
}

constexpr FORCE_INLINE UINT16 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
{
	/* Only valid for 0, 1, 2, or 3. */
	Level = (Level > 3) ? 3 : Level;
	UINT64 Shift = 39 - (Level * 9);
	return (VAddr >> Shift) & 0x1FF;
}

PhysicalAddress VirtualToPhysicalAddress(PageTable *RootTable, VirtualAddress VirtAddr);

VOID InvalidateTLB();

static_assert(sizeof(PageTableEntry) == sizeof(PageTableEntryRaw));


VOID PrintPageTables(pantheon::vmm::PageTable *Table);
VOID PrintPageTablesNoZeroes(pantheon::vmm::PageTable *Table);


class PageAllocator
{
public:
	PageAllocator() : PageAllocator(nullptr, 0) {}

	PageAllocator(VOID *Area, UINT64 Pages)
	{
		OBJECT_SELF_ASSERT();
		this->Allocator = pantheon::mm::SlabCache<pantheon::vmm::PageTable>(Area, Pages);
	}
	
	~PageAllocator() = default;

	pantheon::vmm::PageTable *Allocate()
	{
		OBJECT_SELF_ASSERT();
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

	BOOL Map(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions, BOOL VirtTranslate = TRUE);
	BOOL Reprotect(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions, BOOL VirtTranslate = FALSE);
private:
	static void PanicOnNullPageTable(pantheon::vmm::PageTable *Table)
	{
		if (Table == nullptr)
		{
			StopError("Bad page table (nullptr)\n");
		}
	}

	static void VMMSync()
	{
		pantheon::Sync::DSBISH();
		pantheon::Sync::ISB();		
	}

	static UINT16 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
	{
		/* Only valid for 0, 1, 2, or 3. */
		Level = (Level > 3) ? 3 : Level;
		UINT64 Shift = 39 - (Level * 9);
		return (VAddr >> Shift) & 0x1FF;
	}

	void CreateBlock(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
	{
		OBJECT_SELF_ASSERT();
		Entry->SetRawAttributes(Permissions.GetRawAttributes());
		Entry->SetPhysicalAddressArea(Address);
		Entry->SetBlock(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	void CreateTable(pantheon::vmm::PageTableEntry *Entry, BOOL VirtTranslate = TRUE)
	{
		OBJECT_SELF_ASSERT();
		pantheon::vmm::PageTable *Table = this->Allocate();
		pantheon::vmm::PhysicalAddress PTable = pantheon::CPUReg::R_TTBR1_EL1();
		if (VirtTranslate)
		{
			PTable = VirtualToPhysicalAddress((pantheon::vmm::PageTable*)PTable, (UINT64)Table);
		}
		Entry->SetPhysicalAddressArea(PTable);
		Entry->SetTable(TRUE);
		Entry->SetMapped(TRUE);
		for (pantheon::vmm::PageTableEntry &Entry : Table->Entries)
		{
			Entry.SetRawAttributes(0x00);
		}
		pantheon::Sync::DSBISH();
	}

	BOOL CreateGreedyBlock(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
		OBJECT_SELF_ASSERT();
		if (Size >= BlockSize
			&& IsAligned<UINT64>(VirtAddr, BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize))
		{
			CreateBlock(Entry, Permissions, PhysAddr);
			Size -= BlockSize;
			PhysAddr += BlockSize;
			VirtAddr += BlockSize;
			return TRUE;
		}
		return FALSE;
	}

	pantheon::mm::SlabCache<pantheon::vmm::PageTable> Allocator;
};

}

#endif