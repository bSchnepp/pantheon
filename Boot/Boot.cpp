#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>
#include <PhyProtocol/DeviceTree/DeviceTree.hpp>

#include <Common/Structures/kern_slab.hpp>
#include <Common/Structures/kern_bitmap.hpp>

#include <vmm/vmm.hpp>

#include "Boot.hpp"
#include "mmu.hpp"

#ifndef ONLY_TESTING
extern "C" CHAR *kern_begin;
extern "C" CHAR *kern_end;
#else
static UINT8 Area[50000];
UINT64 kern_begin = (UINT64)&Area;
UINT64 kern_end = (UINT64)(&Area + 50000);
#endif

static InitialBootInfo InitBootInfo;
static UINT64 MemArea = 0x00;
static UINT64 KernSize = 0x00;
static UINT64 KernBegin = 0x00;
static UINT64 KernEnd = 0x00;
static pantheon::Atomic<BOOL> PageTablesCreated = FALSE;
static pantheon::vmm::PageTable *TTBR0 = nullptr;
static pantheon::vmm::PageTable *TTBR1 = nullptr;

InitialBootInfo *GetInitBootInfo()
{
	return &InitBootInfo;
}

void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size);

void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size)
{
	/* If it's less than 4 pages, don't even bother. */
	if (Size < 4 * pantheon::vmm::BlockSize::L4BlockSize)
	{
		return;
	}

	UINT64 ToBitmapPageSize = Size / (8 * pantheon::vmm::BlockSize::L4BlockSize);
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].BaseAddress = (UINT64)StartAddr;
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].Map = pantheon::RawBitmap((UINT8*)MemArea, ToBitmapPageSize);
	MemArea += ToBitmapPageSize;
	InitBootInfo.NumMemoryAreas++;
}

void AllocatePage(UINT64 Addr)
{
	UINT64 NumMemAreas = InitBootInfo.NumMemoryAreas;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		InitialMemoryArea &InitMemArea = InitBootInfo.InitMemoryAreas[InitArea];

		UINT64 MinAddr = InitMemArea.BaseAddress;
		UINT64 MaxAddr = MinAddr + InitMemArea.Map.GetSizeBytes() * pantheon::vmm::BlockSize::L4BlockSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::BlockSize::L4BlockSize);
		InitMemArea.Map.Set(Offset, TRUE);
	}
}

void FreePage(UINT64 Addr)
{
	UINT64 NumMemAreas = InitBootInfo.NumMemoryAreas;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		InitialMemoryArea &InitMemArea = InitBootInfo.InitMemoryAreas[InitArea];

		UINT64 MinAddr = InitMemArea.BaseAddress;
		UINT64 MaxAddr = MinAddr + InitMemArea.Map.GetSizeBytes() * pantheon::vmm::BlockSize::L4BlockSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::BlockSize::L4BlockSize);
		InitMemArea.Map.Set(Offset, FALSE);
	}
}

UINT64 FindPage()
{
	UINT64 NumMemAreas = InitBootInfo.NumMemoryAreas;
	for (UINT64 InitArea = 0; InitArea < NumMemAreas; ++InitArea)
	{
		InitialMemoryArea &InitMemArea = InitBootInfo.InitMemoryAreas[InitArea];
		for (UINT64 Bit = 0; Bit < InitMemArea.Map.GetSizeBits(); ++Bit)
		{
			if (InitMemArea.Map.Get(Bit))
			{
				return InitMemArea.BaseAddress + (pantheon::vmm::BlockSize::L4BlockSize * Bit);
			}
		}
	}
	return 0;
}

void ProcessMemory(DeviceTreeBlob *CurState)
{
	UINT64 Offset = CurState->GetPropStructNameIndex();
	CHAR Buffer[512];
	CHAR Buffer2[512];
	for (UINT32 Index = 0; Index < 512; ++Index)
	{
		Buffer[Index] = '\0';
		Buffer2[Index] = '\0';
	}
	CurState->CopyStringFromOffset(Offset, Buffer, 512);

	if (StringCompare(Buffer, ("reg"), 4))
	{
		/* Assume we'll never need more...? */
		UINT32 Values[64 * 64];

		UINT32 SizeCells = CurState->SizeCells();
		if (SizeCells > 64)
		{
			SizeCells = 64;
		}

		UINT32 AddressCells = CurState->AddressCells();
		if (AddressCells > 64)
		{
			AddressCells = 64;
		}

		for (UINT32 TotalOffset = 0; TotalOffset < SizeCells * AddressCells; ++TotalOffset)
		{
			CurState->CopyU32FromStructPropNode(&Values[TotalOffset], TotalOffset);
		}

		UINT64 Address = 0;
		UINT64 Size = 0;

		for (UINT32 Index = 0; Index < AddressCells; ++Index)
		{
			Address <<= 32;
			Address += Values[Index];
		}

		for (UINT32 Index = 0; Index < SizeCells; ++Index)
		{
			Size <<= 32;
			Size += Values[AddressCells + Index];
		}
		AllocateMemoryArea(Address, Size);
	}
}

void InitializeMemory(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);
	CHAR CurDevNode[512];
	ClearBuffer(CurDevNode, 512);

	while (!DTBState.EndStruct())
	{
		DTBState.NextStruct();
		FDTNodeType CurNode = DTBState.GetStructType();
		if (CurNode == FDT_PROP)
		{
			UINT64 Offset = DTBState.GetPropStructNameIndex();
			CHAR Buffer[512];
			ClearBuffer(Buffer, 512);
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);

			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			if (StringCompare(DevName, "memory", 7) == TRUE)
			{
				ProcessMemory(&DTBState);
			}
			else if (*DevName == '\0')
			{
				UINT64 Offset = DTBState.GetPropStructNameIndex();
				CHAR Buffer[512];
				CHAR Buffer2[512];
				for (UINT32 Index = 0; Index < 512; ++Index)
				{
					Buffer[Index] = '\0';
					Buffer2[Index] = '\0';
				}
				DTBState.CopyStringFromOffset(Offset, Buffer, 512);	

				if (StringCompare(Buffer, ("#size-cells"), 12))
				{
					UINT32 SizeCells;
					DTBState.CopyU32FromStructPropNode(&SizeCells);
					DTBState.SetSizeCells(SizeCells);
				} 
				else if (StringCompare(Buffer, ("#address-cells"), 12))
				{
					UINT32 AddressCells;
					DTBState.CopyU32FromStructPropNode(&AddressCells);
					DTBState.SetAddressCells(AddressCells);
				}
			}
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
		}
	}
}

void Initialize(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);
	CHAR CurDevNode[512];
	ClearBuffer(CurDevNode, 512);

	while (!DTBState.EndStruct())
	{
		DTBState.NextStruct();
		FDTNodeType CurNode = DTBState.GetStructType();
		if (CurNode == FDT_PROP)
		{
			UINT64 Offset = DTBState.GetPropStructNameIndex();
			CHAR Buffer[512];
			ClearBuffer(Buffer, 512);
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				ClearBuffer(Buffer2, 512);
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
			}
			else if (IsU32PropType(Buffer))
			{
				UINT32 U32;
				DTBState.CopyU32FromStructPropNode(&U32);
			}
			else if (IsU64PropType(Buffer))
			{
				UINT64 U64;
				DTBState.CopyU64FromStructPropNode(&U64);
			}
			
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			if (StringCompare(DevName, "memory", 7) == FALSE)
			{
				/* TODO: This should be connecting to a driver init graph... */
				DriverHandleDTB(DevName, &DTBState);
			}
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);

			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			InitDriver(DevName, Addr);

		}
		else if (CurNode == FDT_END_NODE)
		{
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			FiniDriver(DevName, Addr);
		}
	}
}

void PrintDTB(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);
	CHAR CurDevNode[512];
	ClearBuffer(CurDevNode, 512);

	while (!DTBState.EndStruct())
	{
		DTBState.NextStruct();
		FDTNodeType CurNode = DTBState.GetStructType();
		if (CurNode == FDT_PROP)
		{
			UINT64 Offset = DTBState.GetPropStructNameIndex();
			CHAR Buffer[512];
			ClearBuffer(Buffer, 512);
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);
			SERIAL_LOG("%s%s%s", CurDevNode, " : ", Buffer);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				ClearBuffer(Buffer2, 512);
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
				SERIAL_LOG("%s%s%s", " (", Buffer2, ")");
			}
			else if (IsU32PropType(Buffer))
			{
				UINT32 U32;
				DTBState.CopyU32FromStructPropNode(&U32);
				SERIAL_LOG("%s%u%s", " (", U32, ")");
			}
			else if (IsU64PropType(Buffer))
			{
				UINT64 U64;
				DTBState.CopyU64FromStructPropNode(&U64);
				SERIAL_LOG("%s%u%s", " (", U64, ")");
			}
			
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			SERIAL_LOG("%s", "\n");
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
			SERIAL_LOG("%s%s%s", "<<", CurDevNode, ">>\n");

		}
	}
	SERIAL_LOG("%s\n", "finished going through dtb");
}

static void SetupPageTables()
{
	/* As long as we're done after InitializeMemory, we're free to use
	 * any physical memory afterwards.
	 * We just have to:
	 * 	- Ensure we align MemArea to 4K
	 * 	- Add the number of pages to handle there.
	 */

	constexpr UINT64 NumTables = 8192;
	MemArea = Align<UINT64>(MemArea, pantheon::vmm::BlockSize::L4BlockSize);
	pantheon::vmm::PageAllocator InitialPageTables((void*)MemArea, NumTables);

	/* We'll need two top level page tables. */
	TTBR0 = InitialPageTables.Allocate();
	TTBR1 = InitialPageTables.Allocate();

	/* Let's go ahead and map in everything in the lower table as-is. */
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetBlock(TRUE);
	Entry.SetMapped(TRUE);
	Entry.SetUserNoExecute(TRUE);
	Entry.SetKernelNoExecute(FALSE);
	Entry.SetUserAccessible(FALSE);
	Entry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_OUTER);
	Entry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	Entry.SetPagePermissions(pantheon::vmm::PAGE_PERMISSION_KERNEL_RWX);
	Entry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

	/* Begin writing the actual page tables... */
	InitialPageTables.Map(TTBR0, KernBegin, KernBegin, KernSize, Entry);

	/* We need the PMM area as well. */
	UINT64 KernEndAreaAmt = Align<UINT64>(MemArea - KernEnd, pantheon::vmm::BlockSize::L4BlockSize);
	InitialPageTables.Map(TTBR0, KernEnd, KernEnd, KernEndAreaAmt, Entry);

	/* Make sure we're mapping the page tables too. */
	InitialPageTables.Map(TTBR0, MemArea, MemArea, NumTables * sizeof(pantheon::vmm::PageTable), Entry);

	/* 
	 * TODO:
	 * 1. The page table area gets marked as RW.
	 * 2. Remap kernel text as R-X.
	 * 3. Remap kernel rodata as RW-
	 * 4. Remap kernel rwdata as RW-
	 * 5. Reprotect rodata as R--
	 */
	PageTablesCreated.Store(TRUE);
}

static void InstallPageTables()
{
	while (PageTablesCreated.Load() == FALSE)
	{

	}

	UINT64 TTBR0_Val = (UINT64)TTBR0;
	UINT64 TTBR1_Val = (UINT64)TTBR1;

	write_ttbr0_el1(TTBR0_Val);
	write_ttbr1_el1(TTBR1_Val);

	pantheon::vmm::PageTableEntry Entry;
	Entry.SetMapped(TRUE);

	pantheon::arm::MAIRAttributes Attribs = 0;
	Attribs |= pantheon::arm::AttributeToSlot(0x00, 0);
	Attribs |= pantheon::arm::AttributeToSlot(
		pantheon::arm::MAIR_ATTRIBUTE_NORMAL_INNER_NONCACHEABLE | 
		pantheon::arm::MAIR_ATTRIBUTE_NORMAL_OUTER_NONCACHEABLE, 1);

	pantheon::arm::WriteMAIR_EL1(Attribs);
	pantheon::arm::WriteTCR_EL1(pantheon::arm::DefaultTCRAttributes());


}

static void SetupCore()
{
	InstallPageTables();
	pantheon::vmm::InvalidateTLB();
	/* Run pantheon::vmm::EnablePaging() here */
}

extern "C" InitialBootInfo *BootInit(fdt_header *dtb, void *initial_load_addr, void *virt_load_addr)
{
	PANTHEON_UNUSED(initial_load_addr);
	PANTHEON_UNUSED(virt_load_addr);

	pantheon::CPU::CLI();
	UINT8 ProcNo = pantheon::CPU::GetProcessorNumber();
	if (ProcNo == 0)
	{
		UINT64 InitAddr = (UINT64)initial_load_addr;

		KernBegin = InitAddr;
		KernSize = (UINT64)&kern_end - (UINT64)&kern_begin;
		KernEnd = KernBegin + KernSize;

		MemArea = InitAddr + KernSize + pantheon::vmm::BlockSize::L4BlockSize;

		/* The DTB is handled in 3 passes:
		 * The first initializes memory areas, so the physical
		 * memory manager can be started. The second is to prepare
		 * a driver init graph, so the kernel can load drivers as needed.
		 * Lastly, another pass is done to simply print the contents of
		 * the area.
		 */
		InitializeMemory(dtb);

		/* Ensure the appropriate page tables are made */
		SetupPageTables();

		for (UINT64 Start = InitAddr; 
			Start <= Align<UINT64>(MemArea, pantheon::vmm::BlockSize::L4BlockSize);
			Start += pantheon::vmm::BlockSize::L4BlockSize)
		{
			AllocatePage(Start);
		}

	}
	SetupCore();
	
	if (ProcNo == 0)
	{
		/* At this point, paging should be set up so the kernel
		 * gets the page tables expected...
		 */
		BoardInit();
		Initialize(dtb);
		PrintDTB(dtb);		
	}
	return GetInitBootInfo();
}