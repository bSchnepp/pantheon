#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Common/Structures/kern_bitmap.hpp>

#ifndef _PANTHEON_BOOT_HPP_
#define _PANTHEON_BOOT_HPP_

typedef struct InitialMemoryArea
{
	UINT64 BaseAddress;
	pantheon::RawBitmap Map;
}InitialMemoryArea;

typedef struct InitialBootInfo
{
	UINT64 LoadedPhysAddr;
	UINT64 LoadedVirtAddr;

	/* Assume a hard maximum of 64 memory regions. */	
	UINT64 NumMemoryAreas;
	InitialMemoryArea InitMemoryAreas[64];

	PagingInfo PageInfo;
}InitialBootInfo;

InitialBootInfo *GetInitBootInfo();

void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size);
void AllocatePage(UINT64 Addr);
void FreePage(UINT64 Addr);

#endif