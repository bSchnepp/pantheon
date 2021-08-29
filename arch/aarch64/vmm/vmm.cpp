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
	UINT64 Attributes = 0;
	Attributes |= Size;
	Attributes |= Accessor;
	Attributes |= Permission;
	Attributes |= Sharable;
	Attributes |= MMIOType;
	
	return CreateEntry(NextLevel, Attributes);
}

pantheon::vmm::PageTableEntry pantheon::vmm::CreateEntry(const PageTableEntry *NextLevel, UINT64 Attributes)
{
	pantheon::vmm::PageTableEntry Entry = 0;

	UINT64 FinalAddr = (UINT64)NextLevel;
	FinalAddr &= ~0xFFFULL;			/* Wipe lower bits */
	FinalAddr &= ~(0x1FFEULL << 52);	/* Wipe upper bits */
	Entry |= Attributes;
	Entry |= FinalAddr;			/* And write the address in */

	return Entry;
}