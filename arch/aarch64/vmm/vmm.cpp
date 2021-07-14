#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/Memory/kern_physpaging.hpp>

#include "vmm.hpp"

pantheon::vmm::PageTableEntry pantheon::vmm::CreateEntry(
	const PageTableEntry *NextLevel, 
	PageGranularity Size, 
	PageAccessor Accessor, 
	UINT64 Permission, 
	PageSharableType Sharable, 
	PageTypeMMIOAccessor MMIOType)
{
	pantheon::vmm::PageTableEntry Entry = 0;

	UINT64 FinalAddr = (UINT64)NextLevel;
	FinalAddr &= ~0xFFFULL;			/* Wipe lower bits */
	FinalAddr &= ~(0x1FFEULL << 52);	/* Wipe upper bits */
	Entry |= FinalAddr;			/* And write the address in */

	/* For each attribute given, also put that in. */
	Entry |= Size;
	Entry |= Accessor;
	Entry |= Permission;
	Entry |= Sharable;
	Entry |= MMIOType;

	return Entry;
}

pantheon::vmm::PageTable *pantheon::vmm::CreateBasicPageTables()
{
	Optional<UINT64> MaybeAddr = pantheon::GetGlobalPhyManager()->FindAndClaimFirstFreeAddress();
	if (!MaybeAddr.GetOkay())
	{
		return nullptr;
	}
	
	auto *Table = (pantheon::vmm::PageTable *)(MaybeAddr.GetValue());
	ClearBuffer((CHAR*)Table, pantheon::PhyPageManager::PageSize());
	return Table;
}

extern char *TTBR0_AREA;
extern char *TTBR1_AREA;
extern char *TTL2_AREA;
extern char *TTL3_AREA;

static constexpr UINT64 IdentityAttributes()
{
	UINT64 IdentityFlags = 0;
	IdentityFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	IdentityFlags |= pantheon::vmm::PAGE_MISC_ACCESSED;
	IdentityFlags |= pantheon::vmm::PAGE_SHARABLE_TYPE_INNER;
	IdentityFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_KERN;
	IdentityFlags |= pantheon::vmm::PAGE_GRANULARITY_PAGE;
	return IdentityFlags;
}

void initial_paging(void *initial_address)
{
	pantheon::vmm::PageTableEntry *TTBR0 = (pantheon::vmm::PageTableEntry*)(&TTBR0_AREA);
	pantheon::vmm::PageTableEntry *TTBR1 = (pantheon::vmm::PageTableEntry*)(&TTBR1_AREA);
	pantheon::vmm::PageTableEntry *TTL2 = (pantheon::vmm::PageTableEntry*)(&TTL2_AREA);
	pantheon::vmm::PageTableEntry *TTL3 = (pantheon::vmm::PageTableEntry*)(&TTL3_AREA);

	UINT64 InitAddr = (UINT64)initial_address;
	InitAddr &= ~0x3fffffff;	/* 1GB alignment */

	UINT64 IdentityEntry = InitAddr | IdentityAttributes();

	TTBR0[0] = IdentityEntry;
	UINT64 TTBR0Val = ((UINT64)(TTBR0)) | 0x01;
	write_ttbr0_el1(TTBR0Val);

	PANTHEON_UNUSED(TTBR1);
	PANTHEON_UNUSED(TTL2);
	PANTHEON_UNUSED(TTL3);

	PANTHEON_UNUSED(initial_address);
}