#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>
#include <System/Memory/kern_paging.hpp>

pantheon::PhyPageManager::PhyPageManager()
{
	/* FIXME: Correctly base on actual pages! */
	UINT64 MemoryAmt = (8ULL * 1024ULL * 1024ULL * 1024ULL);
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
	UINT64 Index = 0;
	UINT8 CurByte = 0xFF;
	while (CurByte == 0xFF)
	{
		if (Index > this->UsedPages.GetSizeBytes())
		{
			return Optional<UINT64>();
		}

		CurByte = this->UsedPages.GetByte(Index++);
	}

	UINT8 Bit = 0;
	for (Bit = 0; Bit < 8; ++Bit)
	{
		if (this->UsedPages.Get((Index * 8) + Bit) == FALSE)
		{
			break;
		}
	}

	UINT64 PageSz = pantheon::PhyPageManager::PageSize();
	return Optional<UINT64>((PageSz * Bit) * (8 * PageSz * Index));
}

VOID pantheon::PhyPageManager::FreeAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize() * 8;
	this->UsedPages.Set(Addr / AddrDivisor, FALSE);
}

VOID pantheon::PhyPageManager::ClaimAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize() * 8;
	this->UsedPages.Set(Addr / AddrDivisor, TRUE);
}