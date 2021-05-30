#include <kern_macro.hpp>
#include <kern_datatypes.hpp>

#ifndef _AARCH64_MMU_HPP_
#define _AARCH64_MMU_HPP_

namespace pantheon::arm
{

typedef struct PageEntry
{
	union
	{
		UINT64 Raw;
		struct
		{
			UINT8 RESERVED3 : 5;
			UINT8 SW : 4;
			BOOL UXN : 1;
			BOOL PXN : 1;
			UINT64 OutputAddr : 42;
			BOOL AF : 1;
			UINT8 SH : 2;
			BOOL AP : 2;
			BOOL NS : 1;
			UINT8 INDX : 3;
			BOOL TB : 1;
			BOOL VB : 1;
		}__attribute__((packed));
	};
}__attribute__((packed)) PageEntry;

COMPILER_ASSERT(sizeof(PageEntry) == sizeof(UINT64));

}


#endif