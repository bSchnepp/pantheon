#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "vmm.hpp"


static pantheon::vmm::PageTableEntry *GetL3PageTableEntry(const pantheon::vmm::PageTableEntry &Entry, UINT64 VAddr)
{
	UINT64 Tables[4] = {0, 0, 0, 0};
	Tables[3] = (VAddr >> 12) & 0b111111111;
	Tables[2] = (VAddr >> 21) & 0b111111111;
	Tables[1] = (VAddr >> 30) & 0b111111111;
	Tables[0] = (VAddr >> 39) & 0b111111111;

	pantheon::vmm::PageTableEntry *L1 = reinterpret_cast<pantheon::vmm::PageTableEntry*>(pantheon::vmm::PhysicalToVirtualAddress(Entry.GetPhysicalAddressArea()));

	pantheon::vmm::PageTableEntry *L1Entry = &(L1[Tables[1]]);
	if (L1Entry->IsMapped() == FALSE)
	{
		return nullptr;
	}

	pantheon::vmm::PageTableEntry *L2 = reinterpret_cast<pantheon::vmm::PageTableEntry*>(pantheon::vmm::PhysicalToVirtualAddress(L1Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L2Entry = &(L2[Tables[2]]);
	if (L2Entry->IsMapped() == FALSE)
	{
		return nullptr;
	}

	pantheon::vmm::PageTableEntry *L3 = reinterpret_cast<pantheon::vmm::PageTableEntry*>(pantheon::vmm::PhysicalToVirtualAddress(L2Entry->GetPhysicalAddressArea()));
	pantheon::vmm::PageTableEntry *L3Entry = &(L3[Tables[3]]);
	return L3Entry;
}

/**
 * \~english @author Brian Schnepp
 * \~english @brief Looks up a virtual address in either top level page table provided
 * \~english @param Entry The highest level page table entry to look through
 * \~english @param VAddr The virtual address to look up
 * \~english @param Kernel A flag for if the VAddr should be in the TTBR0 or TTBR1 register
 * \~english @details The page table entry supplied is assumed to be the top level page table.
 * In pantheon, only 4Kb pages are used and are strictly presumed to be using 2 sets of 3-level paging for TTBR0 and TTBR1.
 */
BOOL pantheon::vmm::WalkAddr(const pantheon::vmm::PageTableEntry &Entry, UINT64 VAddr)
{
	pantheon::vmm::PageTableEntry *L3Entry = GetL3PageTableEntry(Entry, VAddr);
	if (L3Entry == nullptr || L3Entry->IsMapped() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL pantheon::vmm::MapPages(pantheon::vmm::PageTableEntry &Entry, VirtualAddress VAddr, PhysicalAddress PAddr, pantheon::vmm::PageTableEntry &Permissions)
{
	pantheon::vmm::PageTableEntry *L3Entry = GetL3PageTableEntry(Entry, VAddr);
	if (L3Entry == nullptr || L3Entry->IsMapped() == TRUE)
	{
		return FALSE;
	}

	/* Write in a new entry */
	*L3Entry = Permissions;
	L3Entry->SetPhysicalAddressArea(PAddr);
	return TRUE;
}

BOOL pantheon::vmm::UnmapPages(pantheon::vmm::PageTableEntry &Entry, VirtualAddress VAddr)
{
	pantheon::vmm::PageTableEntry *L3Entry = GetL3PageTableEntry(Entry, VAddr);
	if (L3Entry == nullptr || L3Entry->IsMapped() == TRUE)
	{
		return FALSE;
	}

	/* Clear the entry */
	L3Entry->SetRawAttributes(0);
	return TRUE;
}

VOID pantheon::vmm::EnablePaging()
{
	/* Smash the TLB: we don't want invalid entries left around. */
	asm volatile(
		"dsb ishst\n"
		"tlbi alle1\n"
		"dsb ish\n"
		"isb\n");

	UINT64 SCTLRVal = 0x00;
	asm volatile(
		"mrs %0, sctlr_el1\n"
		"orr %0, %0, #1\n"
		"isb\n" 
		"msr sctlr_el1, %0\n"
		"isb" : "=r"(SCTLRVal)::);
}