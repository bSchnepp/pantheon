#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <Common/Structures/kern_slab.hpp>

#include "vmm.hpp"

static UINT64 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
{
	/* Only valid for 1, 2, 3, or 4. */
	Level = (Level > 4) ? 4 : Level;
	Level = (Level < 1) ? 1 : Level;
	return (VAddr >> (39 - (9 * (Level - 1)))) & 0b111111111;
}

BOOL pantheon::vmm::PageAllocator::Map(pantheon::vmm::PageTable *TTBR, VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
{
	/* Assert that Size is a multiple of the page size. If not, round anyway. */
	Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L4BlockSize);

	/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
	VirtAddr &= ~(pantheon::vmm::BlockSize::L4BlockSize);
	PhysAddr &= ~(pantheon::vmm::BlockSize::L4BlockSize);

	/* We don't have panic yet, so for now just return false. */
	if (TTBR == nullptr)
	{
		return FALSE;
	}

	/* Start mapping everything */
	while (Size > 0)
	{
		/* Can we find the L1 table? */
		UINT8 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
		pantheon::vmm::PageTableEntry *L1Entry = &(TTBR->Entries[L1Index]);

		if (L1Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L1, and write it in. */
			pantheon::vmm::PageTable *Table = Allocator.Allocate();
			L1Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress(TTBR, (UINT64)Table));
			L1Entry->SetMapped(TRUE);
		}

		/* We can't have L1 blocks, so don't try to fit it in.
		 * 512GB pages are a little silly anyway for now.
		 */

		/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
		UINT8 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
		pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);


		namespace BlockSize = pantheon::vmm::BlockSize;
		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (Size >= BlockSize::L2BlockSize 
			&& IsAligned<UINT64>(VirtAddr, BlockSize::L2BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize::L2BlockSize))
		{
			L2Entry->SetRawAttributes(Permissions.GetRawAttributes());
			L2Entry->SetPhysicalAddressArea(PhysAddr);
			Size -= pantheon::vmm::BlockSize::L2BlockSize;
			PhysAddr += pantheon::vmm::BlockSize::L2BlockSize;
			VirtAddr += pantheon::vmm::BlockSize::L2BlockSize;
			L2Entry->SetBlock(TRUE);
			L2Entry->SetMapped(TRUE);
			continue;
		}

		if (L2Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L2, and write it in. */
			pantheon::vmm::PageTable *Table = Allocator.Allocate();
			L2Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress(TTBR, (UINT64)Table));
			L2Entry->SetMapped(TRUE);				
		}

		UINT8 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
		pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (Size >= pantheon::vmm::BlockSize::L3BlockSize
			&& IsAligned<UINT64>(VirtAddr, BlockSize::L3BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize::L3BlockSize))
		{
			L3Entry->SetRawAttributes(Permissions.GetRawAttributes());
			L3Entry->SetPhysicalAddressArea(PhysAddr);
			Size -= pantheon::vmm::BlockSize::L3BlockSize;
			PhysAddr += pantheon::vmm::BlockSize::L3BlockSize;
			VirtAddr += pantheon::vmm::BlockSize::L3BlockSize;
			L3Entry->SetBlock(TRUE);
			L3Entry->SetMapped(TRUE);
			continue;
		}

		if (L3Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L3, and write it in. */
			pantheon::vmm::PageTable *Table = Allocator.Allocate();
			L3Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress(TTBR, (UINT64)Table));
			L3Entry->SetMapped(TRUE);				
		}

		UINT8 L4Index = VirtAddrToPageTableIndex(VirtAddr, 4);
		pantheon::vmm::PageTable *L4 = (pantheon::vmm::PageTable*)(L3Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L4Entry = &(L4->Entries[L4Index]);

		/* We can't make a "bigger" chunk from L4: L4 is already the smallest size. */

		if (L4Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L4, and write it in. */
			L4Entry->SetRawAttributes(Permissions.GetRawAttributes());
			L4Entry->SetPhysicalAddressArea(PhysAddr);
			L4Entry->SetBlock(TRUE);
			L4Entry->SetMapped(TRUE);
		}

		Size -= pantheon::vmm::BlockSize::L4BlockSize;
		PhysAddr += pantheon::vmm::BlockSize::L4BlockSize;
		VirtAddr += pantheon::vmm::BlockSize::L4BlockSize;

	}
	return TRUE;
}

VOID pantheon::vmm::InvalidateTLB()
{
	/* Smash the TLB: we don't want invalid entries left around. */
	asm volatile(
		"isb\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"dsb sy\n"
		"isb\n");
}

VOID pantheon::vmm::EnablePaging()
{
	UINT64 SCTLRVal = 0x00;
	asm volatile("mrs %0, sctlr_el1" : "=r"(SCTLRVal));

	asm volatile(
		"orr %0, %0, #1\n"
		"isb\n" 
		"msr sctlr_el1, %0\n"
		"isb" : "=r"(SCTLRVal)::);
}