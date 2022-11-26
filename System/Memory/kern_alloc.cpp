#include <vmm/vmm.hpp>
#include <vmm/pte.hpp>
#include <Boot/Boot.hpp>
#include <Sync/kern_spinlock.hpp>
#include <System/Memory/kern_alloc.hpp>

static pantheon::Spinlock AllocLock;

struct BitmapRegion
{
	UINT64 BaseAddress;
	pantheon::RawBitmap Map;
};

extern "C" CHAR *PRE_KERN_AREA;
extern "C" CHAR *PRE_KERN_END;
extern "C" CHAR *USER_BEGIN;
extern "C" CHAR *USER_END;
extern "C" CHAR *TEXT_AREA;
extern "C" CHAR *TEXT_PHY_AREA;
extern "C" CHAR *TEXT_END;
extern "C" CHAR *RODATA_AREA;
extern "C" CHAR *RODATA_PHY_AREA;
extern "C" CHAR *RODATA_END;
extern "C" CHAR *DATA_AREA;
extern "C" CHAR *DATA_PHY_AREA;
extern "C" CHAR *DATA_END;
extern "C" CHAR *BSS_AREA;
extern "C" CHAR *BSS_PHY_AREA;
extern "C" CHAR *BSS_END;

/* Assume we average 2GB per reegion, up to NUM_BOOT_MEMORY_AREAS times*/
static UINT64 NumMemArea = 0;
static UINT8 Area[(NUM_BOOT_MEMORY_AREAS * (2ULL * 1024ULL * 1024ULL * 1024ULL)) / (pantheon::vmm::SmallestPageSize * 8)];
static BitmapRegion Regions[NUM_BOOT_MEMORY_AREAS];

bool IsUsed(UINT64 Addr)
{
	for (UINT64 InitArea = 0; InitArea < NumMemArea; ++InitArea)
	{
		UINT64 MinAddr = Regions[InitArea].BaseAddress;
		UINT64 MaxAddr = MinAddr + Regions[InitArea].Map.GetSizeBytes() * pantheon::vmm::SmallestPageSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::SmallestPageSize);
		return Regions[InitArea].Map.Get(Offset);
	}
	return 0;
}

void AllocatePage(UINT64 Addr)
{
	UINT64 NumMemAreas = NumMemArea;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		UINT64 MinAddr = Regions[InitArea].BaseAddress;
		UINT64 MaxAddr = MinAddr + Regions[InitArea].Map.GetSizeBytes() * pantheon::vmm::SmallestPageSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::SmallestPageSize);
		Regions[InitArea].Map.Set(Offset, TRUE);
	}
}

void FreePage(UINT64 Addr)
{
	UINT64 NumMemAreas = NumMemArea;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		UINT64 MinAddr = Regions[InitArea].BaseAddress;
		UINT64 MaxAddr = MinAddr + Regions[InitArea].Map.GetSizeBytes() * pantheon::vmm::SmallestPageSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::SmallestPageSize);
		Regions[InitArea].Map.Set(Offset, FALSE);
	}
}

UINT64 FindPage()
{
	UINT64 NumMemAreas = NumMemArea;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		for (UINT64 Bit = 0; Bit < Regions[InitArea].Map.GetSizeBits(); ++Bit)
		{
			if (Regions[InitArea].Map.Get(Bit) == 0)
			{
				return Regions[InitArea].BaseAddress + (pantheon::vmm::SmallestPageSize * Bit);
			}
		}
	}
	return 0;
}


void pantheon::PageAllocator::InitPageAllocator(InitialBootInfo *BootInfo)
{
	AllocLock = pantheon::Spinlock("page_alloc");
	UINT64 Offset = 0;
	for (UINT64 Index = 0; Index < BootInfo->NumMemoryAreas; Index++)
	{
		Regions[Index].BaseAddress = BootInfo->InitMemoryAreas[Index].BaseAddress;
		Regions[Index].Map = pantheon::RawBitmap(Area + Offset, BootInfo->InitMemoryAreas->Size);
		NumMemArea++;
		Offset += BootInfo->InitMemoryAreas->Size;
	}

#if 0
	pantheon::vmm::PhysicalAddress TextBegin = (pantheon::vmm::PhysicalAddress)&TEXT_PHY_AREA;
	pantheon::vmm::PhysicalAddress TextEnd = TextBegin + ((pantheon::vmm::PhysicalAddress)&TEXT_END - (pantheon::vmm::PhysicalAddress)&TEXT_AREA);

	for (pantheon::vmm::PhysicalAddress Addr = TextBegin; Addr <= TextEnd; Addr += pantheon::vmm::SmallestPageSize)
	{
		AllocatePage(Addr);
	}

	pantheon::vmm::PhysicalAddress RodataBegin = (pantheon::vmm::PhysicalAddress)&RODATA_PHY_AREA;
	pantheon::vmm::PhysicalAddress RodataEnd = RodataBegin + ((pantheon::vmm::PhysicalAddress)&RODATA_END - (pantheon::vmm::PhysicalAddress)&RODATA_AREA);

	for (pantheon::vmm::PhysicalAddress Addr = RodataBegin; Addr <= RodataEnd; Addr += pantheon::vmm::SmallestPageSize)
	{
		AllocatePage(Addr);
	}

	pantheon::vmm::PhysicalAddress DataBegin = (pantheon::vmm::PhysicalAddress)&DATA_PHY_AREA;
	pantheon::vmm::PhysicalAddress DataEnd = DataBegin + ((pantheon::vmm::PhysicalAddress)&DATA_END - (pantheon::vmm::PhysicalAddress)&DATA_AREA);

	for (pantheon::vmm::PhysicalAddress Addr = DataBegin; Addr <= DataEnd; Addr += pantheon::vmm::SmallestPageSize)
	{
		AllocatePage(Addr);
	}
#endif
}

#ifdef ONLY_TESTS
#include <stdlib.h>
#endif

UINT64 pantheon::PageAllocator::Alloc()
{
/* This allows tests to pass in userspace for things like process creation. */
#ifdef ONLY_TESTS
	return (UINT64)malloc(pantheon::vmm::SmallestPageSize);
#endif

	AllocLock.Acquire();
	UINT64 Addr = FindPage();
	AllocatePage(Addr);
	AllocLock.Release();

	if (Addr != 0x00)
	{
		ClearBuffer((CHAR*)pantheon::vmm::PhysicalToVirtualAddress(Addr), pantheon::vmm::SmallestPageSize);
	}

	return Addr;
}

void pantheon::PageAllocator::Free(UINT64 Page)
{
/* This allows tests to pass in userspace for things like process creation. */
#ifdef ONLY_TESTS
	free((void*)Page);
	return;
#endif

	AllocLock.Acquire();
	FreePage(Page);
	AllocLock.Release();
}

bool pantheon::PageAllocator::Used(UINT64 Page)
{
	bool Status = false;
	AllocLock.Acquire();
	Status = !IsUsed(Page);
	AllocLock.Release();
	return Status;
}