#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Common/Structures/kern_bitmap.hpp>

#ifndef _PANTHEON_BOOT_HPP_
#define _PANTHEON_BOOT_HPP_

#define NUM_BOOT_MEMORY_AREAS (16ULL)

typedef struct InitialMemoryArea
{
	UINT64 BaseAddress;
	UINT64 Size;
}InitialMemoryArea;

typedef struct InitialBootInfo
{
	UINT64 LoadedPhysAddr;
	UINT64 LoadedVirtAddr;

	/* Assume a hard maximum of NUM_BOOT_MEMORY_AREAS memory regions. */	
	UINT8 NumMemoryAreas;
	InitialMemoryArea InitMemoryAreas[NUM_BOOT_MEMORY_AREAS];
}InitialBootInfo;

InitialBootInfo *GetInitBootInfo();

#endif