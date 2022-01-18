#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/sync.hpp>
#include <arch/aarch64/vmm/pte.hpp>
#include <Common/Structures/kern_slab.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

extern CHAR *VIRT_LOAD_ADDRESS;

namespace pantheon::vmm
{


FORCE_INLINE VirtualAddress PhysicalToVirtualAddress(PhysicalAddress PhyAddr)
{
	/* TODO */
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
	/* TODO: traverse page table */
	if (RootTable == nullptr)
	{
		return VirtAddr;
	}

	/* Can we find the L0 table? */
	UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
	pantheon::vmm::PageTableEntry *L0Entry = &(RootTable->Entries[L0Index]);

	if (L0Entry->IsMapped() == FALSE)
	{
		return 0;
	}

	UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
	pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L0Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);


	namespace BlockSize = pantheon::vmm::BlockSize;
	if (L1Entry->IsBlock() && L1Entry->IsMapped())
	{
		return L1Entry->GetPhysicalAddressArea();
	}

	UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
	pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L1Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

	if (L2Entry->IsBlock() && L2Entry->IsMapped())
	{
		return L2Entry->GetPhysicalAddressArea();
	}

	UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
	pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L2Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

	if (L3Entry->IsMapped())
	{
		return L3Entry->GetPhysicalAddressArea();
	}
	return VirtAddr;
}

VOID InvalidateTLB();

static_assert(sizeof(PageTableEntry) == sizeof(PageTableEntryRaw));

/* Note that everything in PageAllocator has to be marked as FORCE_INLINE,
 * since this code can also be called in the pre-kernel area 
 * (and thus page tables are not set up yet)
 */

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
		if (Size == 0)
		{
			return TRUE;
		}

		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);
		PhysAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* Start mapping everything */
		while (Size > 0)
		{
			/* Can we find the L0 table? */
			UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);

			if (L0Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L0, and write it in. */
				CreateTable(L0Entry);
			}

			/* We can't have L0 blocks, so don't try to fit it in: this would
			* only be possible for 5-level paging, and 512GB pages are silly
			* right now anyway.
			*/

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L0Entry->GetPhysicalAddressArea());
			if (L1 == nullptr)
			{
				StopError("invalid L1 page table?");
			}			
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);


			namespace BlockSize = pantheon::vmm::BlockSize;
			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L1Entry, Permissions, BlockSize::L1BlockSize))
			{
				continue;
			}

			if (L1Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L1, and write it in. */
				CreateTable(L1Entry);		
			}

			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L1Entry->GetPhysicalAddressArea());
			if (L2 == nullptr)
			{
				StopError("invalid L2 page table?");
			}
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L2Entry, Permissions, BlockSize::L2BlockSize))
			{
				continue;
			}

			if (L2Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L2, and write it in. */
				CreateTable(L2Entry);
			}

			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L2Entry->GetPhysicalAddressArea());
			if (L3 == nullptr)
			{
				StopError("invalid L3 page table?");
			}
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			/* We can't make a "bigger" chunk from L3: L3 is already the smallest size. */

			/* Allocate a new L3, and write it in. */
			CreateBlock(L3Entry, Permissions, PhysAddr);
			L3Entry->SetTable(TRUE);	/* We need this to be 1??? */
			Size -= BlockSize::L3BlockSize;
			PhysAddr += BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;

		}
		VMMSync();
		return TRUE;
	}

	BOOL Reprotect(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		if (Size == 0)
		{
			return TRUE;
		}
			
		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		pantheon::Sync::DSBISH();
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* What indices might we have to look up? */
		UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
		namespace BlockSize = pantheon::vmm::BlockSize;

		/* Start mapping everything */
		while (Size > 0)
		{
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);
			if (L0Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L0Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);

			if (L1Entry == nullptr || L1Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L1Entry->IsBlock())
			{
				/* We'll need to split this block up. */

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L1BlockSize - 1);
				UINT64 Diff = VirtAddr & (BlockSize::L1BlockSize - 1);

				/* These names are to reflect the above. */
				pantheon::vmm::PhysicalAddress PhysMasked = L1Entry->GetPhysicalAddressArea();
				pantheon::vmm::PhysicalAddress PhysAddr = PhysMasked + Diff; 

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L1Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				L1Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->Map(TTBR, VirtMasked, PhysMasked, Diff, OldEntry);
				Status &= this->Map(TTBR, VirtAddr, PhysAddr, Size, Permissions);
				Status &= this->Map(TTBR, VirtAddr + Size, PhysAddr + Size, BlockSize::L1BlockSize - Size - Diff, OldEntry);

				VMMSync();
				return Status;
			}

			/* Otherwise, we need to walk down the table further. */


			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L1Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			if (L2Entry == nullptr || L2Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L2Entry->IsBlock())
			{
				/* We'll need to split this block up. TODO. */
				pantheon::vmm::PhysicalAddress PhysAddr = L2Entry->GetPhysicalAddressArea();

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L2BlockSize - 1);
				UINT64 Diff = VirtAddr - VirtMasked;

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L2Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				/* Next, smash the L2 entry. */
				L2Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->Map(TTBR, VirtMasked, PhysAddr, Diff, OldEntry);
				Status &= this->Map(TTBR, VirtAddr, PhysAddr + Diff, Size, Permissions);
				Status &= this->Map(TTBR, VirtAddr + Size, PhysAddr + Diff + Size, BlockSize::L2BlockSize - Size - Diff, OldEntry);

				return Status;
			}

			/* Otherwise, we need to walk down the table further. */

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress(L2Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			if (L3)
			{
				if (L3Entry->IsMapped() == FALSE)
				{
					/* We don't have something to reprotect: mem doesnt exist. */
					goto fail;
				}

				/* There is no lower level: we'll reprotect this, since it exists. */
				pantheon::vmm::PhysicalAddress PhysAddr = L3Entry->GetPhysicalAddressArea();
				CreateBlock(L3Entry, Permissions, PhysAddr);
				L3Entry->SetTable(TRUE);	/* This is sort of weird: trust that this is right. */
			}
			else
			{
				goto fail;
			}
			
			Size -= BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;
		}
		VMMSync();
		return TRUE;

	fail:
		VMMSync();
		return FALSE;	
	}

	FORCE_INLINE BOOL MapLower(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		if (Size == 0)
		{
			return TRUE;
		}

		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);
		PhysAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* Start mapping everything */
		while (Size > 0)
		{
			/* Can we find the L0 table? */
			UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);

			if (L0Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L0, and write it in. */
				CreateTableLower(L0Entry);
			}

			/* We can't have L0 blocks, so don't try to fit it in: this would
			* only be possible for 5-level paging, and 512GB pages are silly
			* right now anyway.
			*/

			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)(L0Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);


			namespace BlockSize = pantheon::vmm::BlockSize;
			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlockLower(Size, VirtAddr, PhysAddr, L1Entry, Permissions, BlockSize::L1BlockSize))
			{
				continue;
			}

			if (L1Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L1, and write it in. */
				CreateTableLower(L1Entry);		
			}

			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlockLower(Size, VirtAddr, PhysAddr, L2Entry, Permissions, BlockSize::L2BlockSize))
			{
				continue;
			}

			if (L2Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L2, and write it in. */
				CreateTableLower(L2Entry);
			}

			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			/* We can't make a "bigger" chunk from L3: L3 is already the smallest size. */

			/* Allocate a new L3, and write it in. */
			CreateBlockLower(L3Entry, Permissions, PhysAddr);
			L3Entry->SetTable(TRUE);	/* We need this to be 1??? */
			Size -= BlockSize::L3BlockSize;
			PhysAddr += BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;
		}
		VMMSync();
		return TRUE;
	}

	FORCE_INLINE BOOL ReprotectLower(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		if (Size == 0)
		{
			return TRUE;
		}
			
		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		pantheon::Sync::DSBISH();
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* What indices might we have to look up? */
		UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
		namespace BlockSize = pantheon::vmm::BlockSize;

		/* Start mapping everything */
		while (Size > 0)
		{
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);
			if (L0Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)(L0Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);

			if (L1Entry == nullptr || L1Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L1Entry->IsBlock())
			{
				/* We'll need to split this block up. */

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L1BlockSize - 1);
				UINT64 Diff = VirtAddr & (BlockSize::L1BlockSize - 1);

				/* These names are to reflect the above. */
				pantheon::vmm::PhysicalAddress PhysMasked = L1Entry->GetPhysicalAddressArea();
				pantheon::vmm::PhysicalAddress PhysAddr = PhysMasked + Diff; 

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L1Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				L1Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->MapLower(TTBR, VirtMasked, PhysMasked, Diff, OldEntry);
				Status &= this->MapLower(TTBR, VirtAddr, PhysAddr, Size, Permissions);
				Status &= this->MapLower(TTBR, VirtAddr + Size, PhysAddr + Size, BlockSize::L1BlockSize - Size - Diff, OldEntry);

				VMMSync();
				return Status;
			}

			/* Otherwise, we need to walk down the table further. */


			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			if (L2Entry == nullptr || L2Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L2Entry->IsBlock())
			{
				/* We'll need to split this block up. TODO. */
				pantheon::vmm::PhysicalAddress PhysAddr = L2Entry->GetPhysicalAddressArea();

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L2BlockSize - 1);
				UINT64 Diff = VirtAddr - VirtMasked;

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L2Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				/* Next, smash the L2 entry. */
				L2Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->MapLower(TTBR, VirtMasked, PhysAddr, Diff, OldEntry);
				Status &= this->MapLower(TTBR, VirtAddr, PhysAddr + Diff, Size, Permissions);
				Status &= this->MapLower(TTBR, VirtAddr + Size, PhysAddr + Diff + Size, BlockSize::L2BlockSize - Size - Diff, OldEntry);

				return Status;
			}

			/* Otherwise, we need to walk down the table further. */

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			if (L3)
			{
				if (L3Entry->IsMapped() == FALSE)
				{
					/* We don't have something to reprotect: mem doesnt exist. */
					goto fail;
				}

				/* There is no lower level: we'll reprotect this, since it exists. */
				pantheon::vmm::PhysicalAddress PhysAddr = L3Entry->GetPhysicalAddressArea();
				CreateBlockLower(L3Entry, Permissions, PhysAddr);
				L3Entry->SetTable(TRUE);	/* This is sort of weird: trust that this is right. */
			}
			else
			{
				goto fail;
			}
			
			Size -= BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;
		}
		VMMSync();
		return TRUE;

	fail:
		VMMSync();
		return FALSE;	
	}

private:
	static FORCE_INLINE void VMMSync()
	{
		pantheon::Sync::DSBISH();
		pantheon::Sync::ISB();		
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
		Entry->SetRawAttributes(Permissions.GetRawAttributes());
		Entry->SetPhysicalAddressArea(Address);
		Entry->SetBlock(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE void CreateTable(pantheon::vmm::PageTableEntry *Entry)
	{
		pantheon::vmm::PageTable *Table = this->Allocate();
		Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress((pantheon::vmm::PageTable*)pantheon::CPUReg::R_TTBR1_EL1(), (UINT64)Table));
		Entry->SetTable(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE void CreateBlockLower(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
	{
		Entry->SetRawAttributes(Permissions.GetRawAttributes());
		Entry->SetPhysicalAddressArea(Address);
		Entry->SetBlock(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE void CreateTableLower(pantheon::vmm::PageTableEntry *Entry)
	{
		pantheon::vmm::PageTable *Table = this->Allocate();
		Entry->SetPhysicalAddressArea((UINT64)Table);
		Entry->SetTable(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE BOOL CreateGreedyBlock(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
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

	FORCE_INLINE BOOL CreateGreedyBlockLower(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
		if (Size >= BlockSize
			&& IsAligned<UINT64>(VirtAddr, BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize))
		{
			CreateBlockLower(Entry, Permissions, PhysAddr);
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