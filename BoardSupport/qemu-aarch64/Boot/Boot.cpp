#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>
#include <DeviceTree/DeviceTree.hpp>

#include <arch/aarch64/mmu.hpp>
#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/vmm/vmm.hpp>
#include <Common/Structures/kern_slab.hpp>
#include <Common/Structures/kern_bitmap.hpp>

#include "Boot/Boot.hpp"
#include "BootDriver.hpp"

extern "C" CHAR *kern_begin;
extern "C" CHAR *kern_end;
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


static InitialBootInfo InitBootInfo;
static UINT64 MemArea = 0x00;
static UINT64 KernSize = 0x00;
static UINT64 KernBegin = 0x00;
static UINT64 KernEnd = 0x00;
static pantheon::Atomic<BOOL> PageTablesCreated = FALSE;

alignas(0x1000) static pantheon::vmm::PageTable TTBR0;
alignas(0x1000) static pantheon::vmm::PageTable TTBR1;

InitialBootInfo *GetInitBootInfo()
{
	return &InitBootInfo;
}

/* We know memory begins at 0x40000000 and can go for several GB.
 * So, let's just identity map these, and just hope our boot code doesn't
 * need more than 2GB or something (which, if it does, we have very bad problems.)
 */
alignas(0x1000) static pantheon::vmm::PageTable LowerHalfTables[2];

static constexpr UINT64 NumHigherHalfTables = 32;
alignas(0x1000) static pantheon::vmm::PageTable HigherHalfTables[NumHigherHalfTables];

static constexpr pantheon::vmm::PageTableEntry CreateTopLevelPageEntry()
{
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetTable(TRUE);
	Entry.SetMapped(TRUE);
	return Entry;
}

static constexpr pantheon::vmm::PageTableEntry CreateHugePageEntry()
{
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetTable(TRUE);
	Entry.SetMapped(TRUE);

	/* We can use a counter from 0x00 in increments of 0x40000000 
	 * to map 1GB sections at a time. */
	Entry.SetBlock(TRUE);
	Entry.SetMapped(TRUE);
	Entry.SetUserNoExecute(TRUE);
	Entry.SetKernelNoExecute(FALSE);

	return Entry;
}

static constexpr void CreateInitialTables(pantheon::vmm::PageTable *RootTable)
{
	pantheon::vmm::PageTableEntry Entry = CreateTopLevelPageEntry();
	RootTable->Entries[0] = Entry;
	RootTable->Entries[0].SetPhysicalAddressArea((pantheon::vmm::PhysicalAddress)&LowerHalfTables[1]);
	Entry = CreateHugePageEntry();

	/* For now, allow anything since we don't have a real userland yet.
	 * We'll need to create page allocators for userland at some point,
	 * but for now let everything share the same address space.
	 */
	UINT64 PagePermission = 0;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_KERN; 
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_USER;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_USER;
	Entry.SetPagePermissions(PagePermission);

	Entry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
	Entry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	Entry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

	using PhyAddr = pantheon::vmm::PhysicalAddress;
	constexpr PhyAddr OneGB = (1ULL * 1024ULL * 1024ULL * 1024ULL);
	for (PhyAddr Addr = 0x00; Addr < 8 * OneGB; Addr += OneGB)
	{
		UINT8 Index = Addr / OneGB;

		/* Now, to actually map it in. */
		LowerHalfTables[1].Entries[Index] = Entry;
		LowerHalfTables[1].Entries[Index].SetPhysicalAddressArea(Addr);
	}
}

extern "C" void IdentityMapping()
{
	CreateInitialTables(&TTBR0);
}

extern "C" void EnableIdentityMapping()
{
	while (PageTablesCreated.Load() == FALSE)
	{

	}

	UINT64 TTBR0_Val = (UINT64)&TTBR0;
	UINT64 TTBR1_Val = (UINT64)&TTBR1;

	pantheon::CPUReg::W_TTBR0_EL1(TTBR0_Val);
	pantheon::CPUReg::W_TTBR1_EL1(TTBR1_Val);
	pantheon::Sync::ISB();

	pantheon::arm::MAIRAttributes Attribs = 0;
	Attribs |= pantheon::arm::AttributeToSlot(0x00, 0);
	Attribs |= pantheon::arm::AttributeToSlot(
		pantheon::arm::MAIR_ATTRIBUTE_NORMAL_INNER_NONCACHEABLE | 
		pantheon::arm::MAIR_ATTRIBUTE_NORMAL_OUTER_NONCACHEABLE, 1);

	pantheon::CPUReg::W_MAIR_EL1(Attribs);
	pantheon::CPUReg::W_TCR_EL1(pantheon::arm::DefaultTCRAttributes());
	pantheon::Sync::ISB();

	asm volatile(
		"isb\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"dsb sy\n"
		"isb\n" ::: "memory");

	UINT64 SCTLRVal = 0x00;
	asm volatile(
		"isb\n"
		"mrs %0, sctlr_el1\n"
		"isb\n"
		"dsb sy"
		: "=r"(SCTLRVal) :: "memory"
	);

	/* These bits are architecturally required to be set. */
	SCTLRVal |= 0xC00800;

	/* Remove things not desired: thse will probably be helpful later, but not now. */
	UINT64 DisableValues = 0;
	DisableValues |= (pantheon::vmm::SCTLR_EE | pantheon::vmm::SCTLR_E0E);
	DisableValues |= (pantheon::vmm::SCTLR_WXN | pantheon::vmm::SCTLR_I);
	DisableValues |= (pantheon::vmm::SCTLR_SA0 | pantheon::vmm::SCTLR_SA);
	DisableValues |= (pantheon::vmm::SCTLR_C | pantheon::vmm::SCTLR_A);

	SCTLRVal &= ~DisableValues;

	/* The MMU should be enabled though. */
	SCTLRVal |= pantheon::vmm::SCTLR_M;

	asm volatile(
		"isb\n"		
		"msr sctlr_el1, %0\n"
		"isb\n"
		"dsb sy"
		:: "r"(SCTLRVal): "memory");
}


void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size);

void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size)
{
	/* If it's less than 4 pages, don't even bother. */
	if (Size < 4 * pantheon::vmm::SmallestPageSize)
	{
		return;
	}

	UINT64 ToBitmapPageSize = Size / (8 * pantheon::vmm::SmallestPageSize);
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].BaseAddress = (UINT64)StartAddr;
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].Size = Size;
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
		UINT64 MaxAddr = MinAddr + InitMemArea.Map.GetSizeBytes() * pantheon::vmm::SmallestPageSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::SmallestPageSize);
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
		UINT64 MaxAddr = MinAddr + InitMemArea.Map.GetSizeBytes() * pantheon::vmm::SmallestPageSize * 8;

		if (Addr < MinAddr || Addr > MaxAddr)
		{
			continue;
		}

		UINT64 Offset = (Addr - MinAddr) / (pantheon::vmm::SmallestPageSize);
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
			if (InitMemArea.Map.Get(Bit) == 0)
			{
				return InitMemArea.BaseAddress + (pantheon::vmm::SmallestPageSize * Bit);
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

void Initialize(fdt_header *dtb, pantheon::vmm::PageAllocator &Allocator)
{
	/* We'll need this when properly creating device drivers. */
	PANTHEON_UNUSED(Allocator);
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
			SERIAL_LOG_UNSAFE("%s%s%s", CurDevNode, " : ", Buffer);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				ClearBuffer(Buffer2, 512);
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
				SERIAL_LOG_UNSAFE("%s%s%s", " (", Buffer2, ")");
			}
			else if (IsU32PropType(Buffer))
			{
				UINT32 U32;
				DTBState.CopyU32FromStructPropNode(&U32);
				SERIAL_LOG_UNSAFE("%s%u%s", " (", U32, ")");
			}
			else if (IsU64PropType(Buffer))
			{
				UINT64 U64;
				DTBState.CopyU64FromStructPropNode(&U64);
				SERIAL_LOG_UNSAFE("%s%u%s", " (", U64, ")");
			}
			
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			SERIAL_LOG_UNSAFE("%s", "\n");
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
			SERIAL_LOG_UNSAFE("%s%s%s", "<<", CurDevNode, ">>\n");

		}
	}
	SERIAL_LOG_UNSAFE("%s\n", "finished going through dtb");
}

static pantheon::vmm::PageAllocator InitialPageTables;

static void SetupPageTables()
{
	/* As long as we're done after InitializeMemory, we're free to use
	 * any physical memory afterwards.
	 * We just have to:
	 * 	- Ensure we align MemArea to 4K
	 * 	- Add the number of pages to handle there.
	 */
	constexpr UINT64 NumTables = static_cast<UINT64>(NumHigherHalfTables);
	InitialPageTables = pantheon::vmm::PageAllocator((void*)HigherHalfTables, NumTables);

	/* Let's go ahead and map in everything in the lower table as-is. */
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetBlock(TRUE);
	Entry.SetMapped(TRUE);
	Entry.SetUserNoExecute(TRUE);
	Entry.SetKernelNoExecute(FALSE);

	/* For now, allow anything since we don't have a real userland yet.
	 * We'll need to create page allocators for userland at some point,
	 * but for now let everything share the same address space.
	 */
	UINT64 PagePermission = 0;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_KERN; 
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_USER;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_USER;
	Entry.SetPagePermissions(PagePermission);

	Entry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
	Entry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	Entry.SetMAIREntry(pantheon::vmm::MAIREntry_1);


	pantheon::vmm::PageTableEntry UEntry(Entry);
	UEntry.SetPagePermissions(0b01 << 6);
	UEntry.SetKernelNoExecute(TRUE);
	UEntry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
	UEntry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	UEntry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

	UINT64 UAddrBegin = (UINT64)&USER_BEGIN;
	UINT64 UAddrEnd = (UINT64)&USER_END;
	UINT64 UAddrSize = UAddrEnd - UAddrBegin;
	InitialPageTables.Reprotect(&TTBR0, UAddrBegin, UAddrSize, UEntry);

	/* 
	 * TODO:
	 * 0. Utilize TTBR1 for a higher-half mapping, and then
	 * 1. The page table area gets marked as RW.
	 * 2. Remap kernel text as R-X.
	 * 3. Remap kernel rodata as RW-
	 * 4. Remap kernel rwdata as RW-
	 * 5. Reprotect rodata as R-- after running constructors for kernel objects
	 */
	UINT64 NoWritePermission = 0;
	NoWritePermission |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_KERN;
	NoWritePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_KERN; 
	NoWritePermission |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	NoWritePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	pantheon::vmm::PageTableEntry NoWrite(Entry);
	NoWrite.SetPagePermissions(NoWritePermission);

	pantheon::vmm::PhysicalAddress BaseAddrPhysText = reinterpret_cast<pantheon::vmm::PhysicalAddress>(&TEXT_PHY_AREA);
	pantheon::vmm::VirtualAddress BaseAddrText = reinterpret_cast<pantheon::vmm::VirtualAddress>(&TEXT_AREA);
	pantheon::vmm::VirtualAddress EndAddrText = reinterpret_cast<pantheon::vmm::VirtualAddress>(&TEXT_END);
	InitialPageTables.Map(&TTBR1, BaseAddrPhysText, BaseAddrText, EndAddrText - BaseAddrText, NoWrite);

	UINT64 NoExecutePermission = 0;
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN; 
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	pantheon::vmm::PageTableEntry NoExecute(Entry);
	NoExecute.SetPagePermissions(NoExecutePermission);

	pantheon::vmm::PhysicalAddress BaseAddrPhysRodata = reinterpret_cast<pantheon::vmm::PhysicalAddress>(&RODATA_PHY_AREA);
	pantheon::vmm::VirtualAddress BaseAddrRodata = reinterpret_cast<pantheon::vmm::VirtualAddress>(&RODATA_AREA);
	pantheon::vmm::VirtualAddress EndAddrRodata = reinterpret_cast<pantheon::vmm::VirtualAddress>(&RODATA_END);
	InitialPageTables.Map(&TTBR1, BaseAddrPhysRodata, BaseAddrRodata, EndAddrRodata - BaseAddrRodata, NoExecute);

	pantheon::vmm::PhysicalAddress BaseAddrPhysData = reinterpret_cast<pantheon::vmm::PhysicalAddress>(&DATA_PHY_AREA);
	pantheon::vmm::VirtualAddress BaseAddrData = reinterpret_cast<pantheon::vmm::VirtualAddress>(&DATA_AREA);
	pantheon::vmm::VirtualAddress EndAddrData = reinterpret_cast<pantheon::vmm::VirtualAddress>(&DATA_END);
	InitialPageTables.Map(&TTBR1, BaseAddrPhysData, BaseAddrData, EndAddrData - BaseAddrData, NoExecute);

	pantheon::vmm::PhysicalAddress BaseAddrPhysBSS = reinterpret_cast<pantheon::vmm::PhysicalAddress>(&BSS_PHY_AREA);
	pantheon::vmm::VirtualAddress BaseAddrBSS = reinterpret_cast<pantheon::vmm::VirtualAddress>(&BSS_AREA);
	pantheon::vmm::VirtualAddress EndAddrBSS = reinterpret_cast<pantheon::vmm::VirtualAddress>(&BSS_END);
	InitialPageTables.Map(&TTBR1, BaseAddrPhysBSS, BaseAddrBSS, EndAddrBSS - BaseAddrBSS, NoExecute);

	/* Run kernel constructors here */

	UINT64 ROPagePermission = 0;
	ROPagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_KERN;
	ROPagePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN; 
	ROPagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	ROPagePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	Entry.SetPagePermissions(ROPagePermission);
	InitialPageTables.Reprotect(&TTBR1, BaseAddrRodata, EndAddrRodata - BaseAddrRodata, Entry);

	PageTablesCreated.Store(TRUE);
}

pantheon::vmm::PageAllocator *BaseAllocator()
{
	return &InitialPageTables;
}

extern "C" void BoardInit(pantheon::vmm::PageTable *TTBR1, pantheon::vmm::PageAllocator &PageAllocator);

extern "C" InitialBootInfo *BootInit(fdt_header *dtb, void *initial_load_addr, void *virt_load_addr)
{
	PANTHEON_UNUSED(initial_load_addr);
	PANTHEON_UNUSED(virt_load_addr);

	UINT64 ProcNo = 0;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(ProcNo) ::);
	ProcNo &= 0xFF;

	if (ProcNo == 0)
	{
		IdentityMapping();
		/* Ensure the appropriate page tables are made */
		SetupPageTables();		
	}

	EnableIdentityMapping();

	if (ProcNo == 0)
	{
		/* At this point, paging should be set up so the kernel
		 * gets the page tables expected...
		 */
		BoardInit(&TTBR1, InitialPageTables);
		Initialize(dtb, InitialPageTables);
		PrintDTB(dtb);

		UINT64 InitAddr = (UINT64)initial_load_addr;

		KernBegin = InitAddr;
		KernSize = (UINT64)&kern_end - (UINT64)&kern_begin;
		KernEnd = KernBegin + KernSize;

		MemArea = InitAddr + KernSize + pantheon::vmm::SmallestPageSize;

		/* The DTB is handled in 3 passes:
		 * The first initializes memory areas, so the physical
		 * memory manager can be started. The second is to prepare
		 * a driver init graph, so the kernel can load drivers as needed.
		 * Lastly, another pass is done to simply print the contents of
		 * the area.
		 */
		InitializeMemory(dtb);

		for (UINT64 Start = InitAddr; 
			Start <= Align<UINT64>(MemArea, pantheon::vmm::SmallestPageSize);
			Start += pantheon::vmm::SmallestPageSize)
		{
			AllocatePage(Start);
		}

	}
	return GetInitBootInfo();
}