#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <Common/Structures/kern_slab.hpp>

#include "vmm.hpp"
#include <arch.hpp>

static UINT64 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
{
	/* Only valid for 0, 1, 2, or 3. */
	Level = (Level > 3) ? 3 : Level;
	UINT8 Shift = 0;
	switch (Level)
	{
		case 0:
		{
			Shift = 39;
			break;
		}

		case 1:
		{
			Shift = 30;
			break;
		}

		case 2:
		{
			Shift = 21;
			break;
		}

		case 3:
		{
			Shift = 12;
			break;
		}
	}
	return (VAddr >> Shift) & 0x1FF;
}

static void CreateBlock(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
{
	Entry->SetRawAttributes(Permissions.GetRawAttributes());
	Entry->SetPhysicalAddressArea(Address);
	Entry->SetBlock(TRUE);
	Entry->SetMapped(TRUE);
	pantheon::Sync::DSBISH();
}

static void CreateTable(pantheon::vmm::PageTableEntry *Entry, pantheon::vmm::PageTable *TTBR, pantheon::mm::SlabCache<pantheon::vmm::PageTable> &Allocator)
{
	pantheon::vmm::PageTable *Table = Allocator.Allocate();
	Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress(TTBR, (UINT64)Table));
	Entry->SetTable(TRUE);
	Entry->SetMapped(TRUE);
	pantheon::Sync::DSBISH();
}

BOOL pantheon::vmm::PageAllocator::Map(pantheon::vmm::PageTable *TTBR, VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
{
	/* Assert that Size is a multiple of the page size. If not, round anyway. */
	Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

	/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
	VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize);
	PhysAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize);

	/* We don't have panic yet, so for now just return false. */
	if (TTBR == nullptr)
	{
		return FALSE;
	}

	/* Start mapping everything */
	while (Size > 0)
	{
		/* Can we find the L0 table? */
		UINT8 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
		pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);

		if (L0Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L0, and write it in. */
			CreateTable(L0Entry, TTBR, this->Allocator);
		}

		/* We can't have L0 blocks, so don't try to fit it in.
		 * 512GB pages are a little silly anyway for now.
		 */

		/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
		UINT8 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
		pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)(L0Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);


		namespace BlockSize = pantheon::vmm::BlockSize;
		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (Size >= BlockSize::L1BlockSize 
			&& IsAligned<UINT64>(VirtAddr, BlockSize::L1BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize::L1BlockSize))
		{
			CreateBlock(L1Entry, Permissions, PhysAddr);
			Size -= BlockSize::L1BlockSize;
			PhysAddr += BlockSize::L1BlockSize;
			VirtAddr += BlockSize::L1BlockSize;
			continue;
		}

		if (L1Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L1, and write it in. */
			CreateTable(L1Entry, TTBR, this->Allocator);		
		}

		UINT8 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
		pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (Size >= BlockSize::L2BlockSize
			&& IsAligned<UINT64>(VirtAddr, BlockSize::L2BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize::L2BlockSize))
		{
			CreateBlock(L2Entry, Permissions, PhysAddr);
			Size -= BlockSize::L2BlockSize;
			PhysAddr += BlockSize::L2BlockSize;
			VirtAddr += BlockSize::L2BlockSize;
			continue;
		}

		if (L2Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L2, and write it in. */
			CreateTable(L2Entry, TTBR, this->Allocator);
		}

		UINT8 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
		pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
		pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

		/* We can't make a "bigger" chunk from L3: L3 is already the smallest size. */

		/* If this is invalid, map some memory in. */
		if (L3Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L3, and write it in. */
			CreateBlock(L3Entry, Permissions, PhysAddr);
			L3Entry->SetTable(TRUE);	/* We need this to be 1??? */
		}

		Size -= pantheon::vmm::BlockSize::L3BlockSize;
		PhysAddr += BlockSize::L3BlockSize;
		VirtAddr += BlockSize::L3BlockSize;

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

	pantheon::Sync::DSBSY();
	pantheon::Sync::ISB();
	asm volatile(
		"mrs %0, sctlr_el1\n"
		: "=r"(SCTLRVal)
	);

	/* These bits are architecturally required to be set. */
	SCTLRVal |= 0xC00800;

	/* Remove things not desired: thse will probably be helpful later, but not now. */
	SCTLRVal &= ~(pantheon::vmm::SCTLR_EE | pantheon::vmm::SCTLR_E0E);
	SCTLRVal &= ~(pantheon::vmm::SCTLR_WXN | pantheon::vmm::SCTLR_I);
	SCTLRVal &= ~(pantheon::vmm::SCTLR_SA0 | pantheon::vmm::SCTLR_SA);
	SCTLRVal &= ~(pantheon::vmm::SCTLR_C | pantheon::vmm::SCTLR_A);

	/* The MMU should be enabled though. */
	SCTLRVal |= pantheon::vmm::SCTLR_M;

	asm volatile(
		"msr sctlr_el1, %0\n"
		"dsb sy"
		: "=r"(SCTLRVal):: "memory");	
}