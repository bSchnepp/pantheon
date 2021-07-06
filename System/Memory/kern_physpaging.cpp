#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>
#include <Common/Sync/kern_spinlock.hpp>
#include <System/Memory/kern_physpaging.hpp>

/**
 * @file System/Memory/kern_physpaging.cpp
 * \~english @brief Definition for the physical page manager, which records
 * which physical pages are available. Management is split up depending
 * on memory regions available: there is a group manager for each memory
 * area, and then a global manager for the entire address space.
 * \~english @author Brian Schnepp
 */


/**
 * \~english @brief Initializes a basic manager which manages a single page at address 0.
 * \~english @details This constructor should generally only be used for purposes 
 * such as initializing a temporary object, or as the default case in an error. Generally,
 * many systems will consider all memory below 1M to be firmware memory, or "Secure world"
 * memory: it is unsafe to use this memory. As such, usage of this constructor for
 * other purposes should be strongly discouraged.
 * \~english @author Brian Schnepp
 */
pantheon::PhyPageManager::PhyPageManager() : pantheon::PhyPageManager::PhyPageManager(1)
{
}

/**
 * \~english @brief Initializes a basic manager, assumed to be at address 0, with NumPages pages available.
 * \~english @author Brian Schnepp
 * \~english @param[in] NumPages The number of pages which are available at the given address
 */
pantheon::PhyPageManager::PhyPageManager(UINT64 NumPages) :
	pantheon::PhyPageManager::PhyPageManager(0, NumPages)
{
}

/**
 * \~english @brief Initializes a basic manager, with some base address, with NumPages pages available.
 * \~english @author Brian Schnepp
 * \~english @param[in] BaseAddr The first address valid under this manager which can be assigned.
 * \~english @param[in] NumPages The number of pages which are available at the given address
 */
pantheon::PhyPageManager::PhyPageManager(UINT64 BaseAddr, UINT64 NumPages)
{
	this->UsedPages = pantheon::Bitmap(NumPages / 8);
	this->BaseAddress = BaseAddr;
	if (BaseAddr % pantheon::PhyPageManager::PageSize())
	{
		this->BaseAddress -= BaseAddr % pantheon::PhyPageManager::PageSize();
	}
}


pantheon::PhyPageManager::~PhyPageManager()
{
	
}

/**
 * \~english @brief Obtains the smallest native memory page size for the current platform
 * \~english @author Brian Schnepp
 */
UINT64 pantheon::PhyPageManager::PageSize()
{
	/* For now, assume all platforms are using 4K pages. */
	return 4096;
}

/**
 * \~english @brief Finds the first free address this manager can allocate
 * \~english @author Brian Schnepp
 * \~english @return The address found inside an Optional, if found. If none found, a None optional is returned instead.
 */
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

/**
 * \~english @brief Releases control over a specific page given.
 * \~english @author Brian Schnepp
 * \~english @param[in] Addr The address to release control of
 */
VOID pantheon::PhyPageManager::FreeAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set((Addr - this->BaseAddress) / AddrDivisor, FALSE);
}

/**
 * \~english @brief Marks a given page as being in use.
 * \~english @author Brian Schnepp
 * \~english @param[in] Addr The address to claim control of
 */
VOID pantheon::PhyPageManager::ClaimAddress(UINT64 Addr)
{
	UINT64 AddrDivisor = pantheon::PhyPageManager::PageSize();
	this->UsedPages.Set((Addr - this->BaseAddress) / AddrDivisor, TRUE);
}

/**
 * \~english @brief Gets the first valid page which this manager can allocate
 * \~english @author Brian Schnepp
 * \~english @return The first address this manager can offer. Always aligned to at least the smallest native page size.
 */
[[nodiscard]] 
UINT64 pantheon::PhyPageManager::BaseAddr() const
{
	return this->BaseAddress;
}

/**
 * \~english @brief Gets the first valid page which this manager can allocate
 * \~english @author Brian Schnepp
 * \~english @return The first address this manager can offer. Always aligned to at least the smallest native page size.
 */
[[nodiscard]] 
UINT64 pantheon::PhyPageManager::NumPages() const
{
	return this->UsedPages.GetSizeBits();
}

static pantheon::Spinlock GlobalAccessorLock;

/**
 * \~english @brief Initializes the global page manager, which manages the entire address space
 * \~english @author Brian Schnepp
 */
pantheon::GlobalPhyPageManager::GlobalPhyPageManager()
{
}

/**
 * \~english @brief Notifies the global manager about a memory area not yet managed.
 * \~english @param[in] BaseAddress The address of the first allocatable page in this area
 * \~english @param[in] NumPages The number of pages this area contains, in units of the smallest allocatable page size.
 * \~english @author Brian Schnepp
 * 
 * @see pantheon::PhyPageManager::PageSize
 */
VOID pantheon::GlobalPhyPageManager::AddArea(UINT64 BaseAddress, UINT64 NumPages)
{
	GlobalAccessorLock.Acquire();
	this->Managers.Add(pantheon::PhyPageManager(BaseAddress, NumPages));
	GlobalAccessorLock.Release();
}

/**
 * \~english @brief Finds the first page not marked as in use which can be allocated
 * \~english @author Brian Schnepp
 * \~english @return The address found inside an Optional, if found. If none found, a None optional is returned instead.
 */
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

/**
 * \~english @brief Marks a given page address as no longer in use
 * \~english @param[in] Addr The address of the page to mark as free, aligned to the smallest page size
 * \~english @author Brian Schnepp
 * 
 * @see pantheon::PhyPageManager::PageSize
 */
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

/**
 * \~english @brief Marks a given page address as in use
 * \~english @param[in] Addr The address of the page to mark as used, aligned to the smallest page size
 * \~english @author Brian Schnepp
 * 
 * @see pantheon::PhyPageManager::PageSize
 */
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

/**
 * \~english @brief Obtains the GlobalPhyPageManager singleton
 * \~english @return A pointer to the GlobalPhyPageManager for this system
 * \~english @author Brian Schnepp
 */
pantheon::GlobalPhyPageManager *pantheon::GetGlobalPhyManager()
{
	if (!GlobalManager)
	{
		GlobalManager = (pantheon::GlobalPhyPageManager*)BasicMalloc(sizeof(pantheon::GlobalPhyPageManager))();
		*GlobalManager = pantheon::GlobalPhyPageManager();
	}
	return GlobalManager;
}