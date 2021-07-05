#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>
#include <System/Memory/kern_physpaging.hpp>

pantheon::PhyPageManager::PhyPageManager()
{
	/* FIXME: Correctly base on actual pages! */
	UINT64 MemoryAmt = (4096 * 18);
	this->UsedPages = pantheon::Bitmap(MemoryAmt / pantheon::PhyPageManager::PageSize());
}

pantheon::PhyPageManager::~PhyPageManager()
{
	
}

UINT64 pantheon::PhyPageManager::PageSize()
{
	/* Assume all pages are the same size, and that is 4096 bytes. */
	return 4096;
}

Optional<UINT64> pantheon::PhyPageManager::FindFreeAddress()
{
	UINT64 Bit = 0;
	while (Bit < this->UsedPages.GetSizeBits())
	{
		if (this->UsedPages.Get(Bit) == FALSE)
		{
			return Optional<UINT64>(pantheon::PhyPageManager::PageSize() * Bit);
		}
		Bit++;
	}
	return Optional<UINT64>();
}

VOID pantheon::PhyPageManager::FreeAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set(Addr / AddrDivisor, FALSE);
}

VOID pantheon::PhyPageManager::ClaimAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set(Addr / AddrDivisor, TRUE);
}