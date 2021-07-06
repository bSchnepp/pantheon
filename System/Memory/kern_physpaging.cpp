#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>
#include <Common/Sync/kern_spinlock.hpp>
#include <System/Memory/kern_physpaging.hpp>

pantheon::PhyPageManager::PhyPageManager()
{
	/* FIXME: Correctly base on actual pages! */
	UINT64 MemoryAmt = (4096 * 18);
	this->UsedPages = pantheon::Bitmap(MemoryAmt);
}

pantheon::PhyPageManager::PhyPageManager(UINT64 NumPages) :
	pantheon::PhyPageManager::PhyPageManager(0, NumPages)
{
}

pantheon::PhyPageManager::PhyPageManager(UINT64 BaseAddr, UINT64 NumPages)
{
	this->UsedPages = pantheon::Bitmap(NumPages / 8);
	this->BaseAddress = BaseAddr;
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
			UINT64 FinalAddr = pantheon::PhyPageManager::PageSize() * Bit;
			return Optional<UINT64>(this->BaseAddress + FinalAddr);
		}
		Bit++;
	}
	return Optional<UINT64>();
}

VOID pantheon::PhyPageManager::FreeAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set((Addr - this->BaseAddress) / AddrDivisor, FALSE);
}

VOID pantheon::PhyPageManager::ClaimAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set((Addr - this->BaseAddress) / AddrDivisor, TRUE);
}
[[nodiscard]] 
UINT64 pantheon::PhyPageManager::BaseAddr() const
{
	return this->BaseAddress;
}

[[nodiscard]] 
UINT64 pantheon::PhyPageManager::NumPages() const
{
	return this->UsedPages.GetSizeBits();
}

static pantheon::Spinlock GlobalAccessorLock;

 pantheon::GlobalPhyPageManager::GlobalPhyPageManager()
 {
 }

VOID pantheon::GlobalPhyPageManager::AddArea(UINT64 BaseAddress, UINT64 NumPages)
{
	GlobalAccessorLock.Acquire();
	this->Managers.Add(pantheon::PhyPageManager(BaseAddress, NumPages));
	GlobalAccessorLock.Release();
}

Optional<UINT64> pantheon::GlobalPhyPageManager::FindFreeAddress()
{
	GlobalAccessorLock.Acquire();
	for (pantheon::PhyPageManager &Manager : this->Managers)
	{
		Optional<UINT64> Addr = Manager.FindFreeAddress();
		if (Addr.GetOkay())
		{
			return Addr;
		}
	}
	GlobalAccessorLock.Release();
	return Optional<UINT64>();
}

VOID pantheon::GlobalPhyPageManager::FreeAddress(UINT64 Addr)
{
	GlobalAccessorLock.Acquire();
	for (pantheon::PhyPageManager &Manager : this->Managers)
	{
		UINT64 BaseAddr = Manager.BaseAddr();
		UINT64 AreaSize = Manager.NumPages() * pantheon::PhyPageManager::PageSize();
		if (Addr >= BaseAddr && Addr <= BaseAddr + AreaSize)
		{
			Manager.FreeAddress(Addr);
		}
	}
	GlobalAccessorLock.Release();
}

VOID pantheon::GlobalPhyPageManager::ClaimAddress(UINT64 Addr)
{
	GlobalAccessorLock.Acquire();
	for (pantheon::PhyPageManager &Manager : this->Managers)
	{
		UINT64 BaseAddr = Manager.BaseAddr();
		UINT64 AreaSize = Manager.NumPages() * pantheon::PhyPageManager::PageSize();
		if (Addr >= BaseAddr && Addr <= BaseAddr + AreaSize)
		{
			Manager.ClaimAddress(Addr);
		}
	}
	GlobalAccessorLock.Release();
}

static pantheon::GlobalPhyPageManager *GlobalManager = nullptr;

pantheon::GlobalPhyPageManager *pantheon::GetGlobalPhyManager()
{
	if (!GlobalManager)
	{
		GlobalManager = (pantheon::GlobalPhyPageManager*)BasicMalloc(sizeof(pantheon::GlobalPhyPageManager))();
		*GlobalManager = pantheon::GlobalPhyPageManager();
	}
	return GlobalManager;
}