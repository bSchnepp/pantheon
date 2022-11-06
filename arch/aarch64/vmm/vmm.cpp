#include "vmm.hpp"
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <Common/Structures/kern_slab.hpp>

VOID pantheon::vmm::InvalidateTLB()
{
	/* Smash the TLB: we don't want invalid entries left around. */
	asm volatile(
		"isb\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"dsb sy\n"
		"isb\n" ::: "memory");
}

VOID pantheon::vmm::PrintPageTables(pantheon::vmm::PageTable *Table)
{
	for (const pantheon::vmm::PageTableEntry &L0Entry : Table->Entries)
	{
		SERIAL_LOG("0x%lx\n", L0Entry.GetRawAttributes());
		if (L0Entry.IsMapped())
		{
			pantheon::vmm::PageTable *L1 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L0Entry.GetPhysicalAddressArea()));
			for (const pantheon::vmm::PageTableEntry &L1Entry : L1->Entries)
			{
				SERIAL_LOG("\t0x%lx\n", L1Entry.GetRawAttributes());
				if (L1Entry.IsMapped())
				{
					pantheon::vmm::PageTable *L2 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L1Entry.GetPhysicalAddressArea()));
					for (const pantheon::vmm::PageTableEntry &L2Entry : L2->Entries)
					{
						SERIAL_LOG("\t\t0x%lx\n", L2Entry.GetRawAttributes());
						if (L2Entry.IsMapped())
						{
							pantheon::vmm::PageTable *L3 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L2Entry.GetPhysicalAddressArea()));
							for (const pantheon::vmm::PageTableEntry &L3Entry : L3->Entries)
							{
								SERIAL_LOG("\t\t\t0x%lx\n", L3Entry.GetRawAttributes());
							}
						}
					}
				}
			}
		}
	}
}

VOID pantheon::vmm::PrintPageTablesNoZeroes(pantheon::vmm::PageTable *Table)
{
	UINT64 L0Index = 0;
	for (const pantheon::vmm::PageTableEntry &L0Entry : Table->Entries)
	{
		if (L0Entry.GetRawAttributes() != 0)
		{
			SERIAL_LOG("0x%lx\n", L0Entry.GetRawAttributes());
		}
		if (L0Entry.IsMapped())
		{
			UINT64 L1Index = 0;
			pantheon::vmm::PageTable *L1 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L0Entry.GetPhysicalAddressArea()));
			for (const pantheon::vmm::PageTableEntry &L1Entry : L1->Entries)
			{
				if (L1Entry.GetRawAttributes() != 0x00)
				{
					SERIAL_LOG("\t0x%lx\n", L1Entry.GetRawAttributes());
				}

				if (L1Entry.IsMapped())
				{
					UINT64 L2Index = 0;
					pantheon::vmm::PageTable *L2 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L1Entry.GetPhysicalAddressArea()));
					for (const pantheon::vmm::PageTableEntry &L2Entry : L2->Entries)
					{
						if (L2Entry.GetRawAttributes() != 0x00)
						{
							SERIAL_LOG("\t\t0x%lx\n", L2Entry.GetRawAttributes());
						}
						if (L2Entry.IsMapped())
						{
							UINT64 L3Index = 0;
							pantheon::vmm::PageTable *L3 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L2Entry.GetPhysicalAddressArea()));
							for (const pantheon::vmm::PageTableEntry &L3Entry : L3->Entries)
							{
								if (L3Entry.GetRawAttributes() != 0x00)
								{
									UINT64 VirtAddress = 0x00;
									VirtAddress |= (L0Index << ((3 * 9) + 12));
									VirtAddress |= (L1Index << ((2 * 9) + 12));
									VirtAddress |= (L2Index << ((1 * 9) + 12));
									VirtAddress |= (L3Index << ((0 * 9) + 12));

									SERIAL_LOG("\t\t\t0x%lx\t[mapped at 0x%lx, index %ld %ld %ld %ld]\n", L3Entry.GetRawAttributes(), VirtAddress, L0Index, L1Index, L2Index, L3Index);
								}
								L3Index++;
							}
						}
						L2Index++;
					}
				}
				L1Index++;
			}
			L0Index++;
		}
	}
}

pantheon::vmm::PhysicalAddress pantheon::vmm::VirtualToPhysicalAddress(pantheon::vmm::PageTable *RootTable, pantheon::vmm::VirtualAddress VirtAddr)
{
	/* TODO: traverse page table */
	if (RootTable == nullptr)
	{
		return VirtAddr;
	}

	/* Can we find the L0 table? */
	UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
	UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
	UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
	UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);


	pantheon::vmm::PageTableEntry *L0Entry = &(RootTable->Entries[L0Index]);

	if (L0Entry->IsMapped() == FALSE)
	{
		return 0;
	}

	namespace BlockSize = pantheon::vmm::BlockSize;

	pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L0Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);

	if (L1Entry->IsBlock() && L1Entry->IsMapped())
	{
		/* The mask for an L1 table is 30 bits: */
		pantheon::vmm::VirtualAddress Mask = pantheon::vmm::BlockSize::L1BlockSize - 1;
		pantheon::vmm::PhysicalAddress Offset = VirtAddr & Mask;
		return L1Entry->GetPhysicalAddressArea() + Offset;
	}

	pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L1Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

	if (L2Entry->IsBlock() && L2Entry->IsMapped())
	{
		/* The mask for an L2 table is 21 bits: */
		pantheon::vmm::VirtualAddress Mask = pantheon::vmm::BlockSize::L2BlockSize - 1;
		pantheon::vmm::PhysicalAddress Offset = VirtAddr & Mask;
		return L2Entry->GetPhysicalAddressArea() + Offset;
	}

	pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((L2Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

	if (L3Entry->IsMapped())
	{
		/* The mask for an L2 table is 12 bits: */
		pantheon::vmm::VirtualAddress Mask = pantheon::vmm::BlockSize::L3BlockSize - 1;
		pantheon::vmm::PhysicalAddress Offset = VirtAddr & Mask;
		return L3Entry->GetPhysicalAddressArea() + Offset;
	}

	return 0x00;
}

BOOL pantheon::vmm::PageAllocator::Map(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions, BOOL VirtTranslate)
{
	OBJECT_SELF_ASSERT();
	if (Size == 0)
	{
		return TRUE;
	}

	/* TTBR is L0 table */
	PanicOnNullPageTable(TTBR);

	/* Assert that Size is a multiple of the page size. If not, round anyway. */
	Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

	/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
	VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);
	PhysAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

	if (TTBR == nullptr)
	{
		StopErrorFmt("Bad page table root: %lx\n", TTBR);
	}

	/* Start mapping everything */
	while (Size > 0)
	{
		/* Can we find the L0 table? */
		UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
		UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
		UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
		UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);

		pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);
		if (L0Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L0, and write it in. */
			CreateTable(L0Entry, VirtTranslate);
		}

		/* We can't have L0 blocks, so don't try to fit it in: this would
		* only be possible for 5-level paging, and 512GB pages are silly
		* right now anyway.
		*/

		pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)L0Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L1);
		}
		PanicOnNullPageTable(L1);
		pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);

		if (TTBR == L1)
		{
			StopError("Bad overlapping page tables (L0 == L1)\n");
		}


		namespace BlockSize = pantheon::vmm::BlockSize;
		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L1Entry, Permissions, BlockSize::L1BlockSize))
		{
			continue;
		}

		if (L1Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L1, and write it in. */
			CreateTable(L1Entry, VirtTranslate);		
		}

		pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)L1Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L2);
		}
		PanicOnNullPageTable(L2);
		pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

		if (L1 == L2)
		{
			StopError("Bad overlapping page tables (L1 == L2)\n");
		}

		/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
		if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L2Entry, Permissions, BlockSize::L2BlockSize))
		{
			continue;
		}

		if (L2Entry->IsMapped() == FALSE)
		{
			/* Allocate a new L2, and write it in. */
			CreateTable(L2Entry, VirtTranslate);
		}

		pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)L2Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L3);
		}
		PanicOnNullPageTable(L3);
		pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

		if (L2 == L3)
		{
			StopError("Bad overlapping page tables (L2 == L3)\n");
		}

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

BOOL pantheon::vmm::PageAllocator::Reprotect(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions, BOOL VirtTranslate)
{
	OBJECT_SELF_ASSERT();
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
		pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)L0Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L1 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L1);
		}
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
		pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)L1Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L2 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L2);
		}
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
		pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)L2Entry->GetPhysicalAddressArea();
		if (VirtTranslate)
		{
			L3 = (pantheon::vmm::PageTable*)PhysicalToVirtualAddress((pantheon::vmm::PhysicalAddress)L3);
		}
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