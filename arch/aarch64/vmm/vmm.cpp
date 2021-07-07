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