#include <arch.hpp>
#include <byte_swap.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Common/Structures/kern_bitmap.hpp>
#include <System/Exec/kern_elf_relocations.hpp>

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

typedef struct KernelHeader
{
	UINT32 JumpInstr;
	CHAR Signature[8];
	UINT32 TextBegin;
	UINT32 TextEnd;
	UINT32 RodataBegin;
	UINT32 RodataEnd;
	UINT32 DataBegin;
	UINT32 DataEnd;
	UINT32 BSSBegin;
	UINT32 BSSEnd;
	UINT32 InitArrayBegin;
	UINT32 InitArrayEnd;
	UINT64 Dynamic; 
}__attribute__((__packed__)) KernelHeader;


static_assert(sizeof(KernelHeader) == 60);

InitialBootInfo *GetInitBootInfo();
void *GetBootStackArea(UINT64 Core);

#define PHYSICAL_MAP_AREA_ADDRESS (0xFFFF000000000000)

#endif