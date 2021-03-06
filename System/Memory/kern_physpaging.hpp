#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>

#include <kern_container.hpp>

#include <Sync/kern_spinlock.hpp>

#ifndef _KERN_PHYSPAGING_HPP_
#define _KERN_PHYSPAGING_HPP_

namespace pantheon
{

class PhyPageManager
{
public:
	PhyPageManager();
	PhyPageManager(UINT64 NumPages);
	PhyPageManager(UINT64 BaseAddress, UINT64 NumPages);
	~PhyPageManager();

	Optional<UINT64> FindFreeAddress();
	VOID FreeAddress(UINT64 Addr);
	VOID ClaimAddress(UINT64 Addr);

	BOOL CheckClaimed(UINT64 Address);

	static UINT64 PageSize();
	[[nodiscard]] UINT64 BaseAddr() const;
	[[nodiscard]] UINT64 NumPages() const;

private:
	UINT64 BaseAddress;
	pantheon::Bitmap UsedPages;

};

class GlobalPhyPageManager
{

public:
	GlobalPhyPageManager();
	VOID AddArea(UINT64 BaseAddress, UINT64 NumPages);

	Optional<UINT64> FindFreeAddress();
	VOID FreeAddress(UINT64 Addr);
	VOID ClaimAddress(UINT64 Addr);

	BOOL CheckClaimed(UINT64 Address);

	Optional<UINT64> FindAndClaimFirstFreeAddress();

	UINT64 HighestAddress();

private:
	pantheon::Spinlock GlobalAccessorLock;
	ArrayList<PhyPageManager> Managers;
};

VOID InitGlobalPhyPageManagers();
GlobalPhyPageManager *GetGlobalPhyManager();
GlobalPhyPageManager *GetGlobalMMIOManager();

}

#endif