#include <kern_datatypes.hpp>
#include <Structures/kern_bitmap.hpp>
#include <Structures/kern_optional.hpp>

#ifndef _KERN_PAGING_HPP_
#define _KERN_PAGING_HPP_

namespace pantheon
{

class PhyPageManager
{
public:
	PhyPageManager();
	~PhyPageManager();

	Optional<UINT64> FindFreeAddress();
	VOID FreeAddress(UINT64 Addr);
	VOID ClaimAddress(UINT64 Addr);

	static UINT64 PageSize();

private:
	pantheon::Bitmap UsedPages;

};

}

#endif