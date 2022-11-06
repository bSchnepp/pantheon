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
#include "Boot/BootDriver.hpp"

#include <Common/Sync/kern_atomic.hpp>
#include <System/Exec/kern_elf.hpp>


extern "C" CHAR *kernel_location;


static InitialBootInfo InitBootInfo;
static pantheon::Atomic<BOOL> PageTablesCreated = FALSE;

alignas(0x1000) static pantheon::vmm::PageTable TTBR0;
alignas(0x1000) static pantheon::vmm::PageTable TTBR1;

static constexpr UINT64 NumHigherHalfTables = 128;
alignas(0x1000) static pantheon::vmm::PageTable LowerHalfTables[2];
alignas(0x1000) static pantheon::vmm::PageTable HigherHalfTables[NumHigherHalfTables];
alignas(0x1000) static char BootStackArea[MAX_NUM_CPUS * DEFAULT_STACK_SIZE];

InitialBootInfo *GetInitBootInfo()
{
	return &InitBootInfo;
}

namespace pantheon
{
	void StopError(const char *Name, void *Info)
	{
		PANTHEON_UNUSED(Name);
		PANTHEON_UNUSED(Info);
		for (;;) {}
	}
	
	void StopErrorFmt(const char *Name, ...)
	{
		PANTHEON_UNUSED(Name);
		for (;;) {}
	}
}

void SERIAL_LOG(const char *c, ...)
{
	PANTHEON_UNUSED(c);
	/* Silence linking problems, FIXME! */
}

void *GetBootStackArea(UINT64 Core)
{
	return BootStackArea + static_cast<UINT64>(Core * DEFAULT_STACK_SIZE);
}


static void CreateInitialTables(pantheon::vmm::PageTable *RootTable)
{
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetTable(TRUE);
	Entry.SetMapped(TRUE);

	RootTable->Entries[0] = Entry;
	RootTable->Entries[0].SetPhysicalAddressArea((pantheon::vmm::PhysicalAddress)&LowerHalfTables[1]);

	/* We can use a counter from 0x00 in increments of 0x40000000 
	 * to map 1GB sections at a time. */
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

void IdentityMapping()
{
	CreateInitialTables(&TTBR0);
}

void EnableIdentityMapping()
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

void AllocateMemoryArea(UINT64 StartAddr, UINT64 Size)
{
	/* If it's less than 4 pages, don't even bother. */
	if (Size < 4 * pantheon::vmm::SmallestPageSize)
	{
		return;
	}

	if (InitBootInfo.NumMemoryAreas == NUM_BOOT_MEMORY_AREAS)
	{
		/* We're out of room. give up. */
		return;
	}

	UINT64 ToBitmapPageSize = Size / (8 * pantheon::vmm::SmallestPageSize);
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].BaseAddress = (UINT64)StartAddr;
	InitBootInfo.InitMemoryAreas[InitBootInfo.NumMemoryAreas].Size = (ToBitmapPageSize + 1);
	InitBootInfo.NumMemoryAreas++;
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

static pantheon::vmm::PageAllocator InitialPageTables;
static UINT64 VirtualAddress = 0xFFFFFFFF70000000;

static void SetupPageTables()
{
	pantheon::CPUReg::W_TTBR1_EL1(0);
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

	KernelHeader Hdr;
	memcpy(&Hdr, (void*)&kernel_location, sizeof(KernelHeader));

	pantheon::vmm::PhysicalAddress KPAddr = (pantheon::vmm::PhysicalAddress)&kernel_location;
	pantheon::vmm::PhysicalAddress KPTextArea = KPAddr + Hdr.TextBegin;
	pantheon::vmm::PhysicalAddress KPTextEnd = KPAddr + Hdr.TextEnd;
	pantheon::vmm::PhysicalAddress KPRodataArea = KPAddr + Hdr.RodataBegin;
	pantheon::vmm::PhysicalAddress KPRodataEnd = KPAddr + Hdr.RodataEnd;
	pantheon::vmm::PhysicalAddress KPDataArea = KPAddr + Hdr.DataBegin;
	pantheon::vmm::PhysicalAddress KPDataEnd = KPAddr + Hdr.DataEnd;
	pantheon::vmm::PhysicalAddress KPBssArea = KPAddr + Hdr.BSSBegin;
	pantheon::vmm::PhysicalAddress KPBssEnd = KPAddr + Hdr.BSSEnd;

	pantheon::vmm::VirtualAddress KVText = VirtualAddress + Hdr.TextBegin;
	pantheon::vmm::VirtualAddress KVRodata = VirtualAddress + Hdr.RodataBegin;
	pantheon::vmm::VirtualAddress KVData = VirtualAddress + Hdr.DataBegin;
	pantheon::vmm::VirtualAddress KVBss = VirtualAddress + Hdr.BSSBegin;

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

	InitialPageTables.Map(&TTBR1, KVText, KPTextArea, KPTextEnd - KPTextArea, NoWrite);

	UINT64 NoExecutePermission = 0;
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN; 
	NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	pantheon::vmm::PageTableEntry NoExecute(Entry);
	NoExecute.SetPagePermissions(NoExecutePermission);

	InitialPageTables.Map(&TTBR1, KVRodata, KPRodataArea, KPRodataEnd - KPRodataArea, NoExecute);
	InitialPageTables.Map(&TTBR1, KVData, KPDataArea, KPDataEnd - KPDataArea, NoExecute);
	InitialPageTables.Map(&TTBR1, KVBss, KPBssArea, KPBssEnd - KPBssArea, NoExecute);
	PageTablesCreated.Store(TRUE);
}

extern "C" void BoardInit(pantheon::vmm::PageTable *TTBR1, pantheon::vmm::PageAllocator &PageAllocator);

static KernelHeader ExtractKernelHeader()
{
	KernelHeader Hdr;
	memcpy(&Hdr, (void*)&kernel_location, sizeof(KernelHeader));

	/* Verify that the boot image is valid */
	const CHAR *Str = "PANTHEON";
	for (uint8_t Index = 0; Index < 8; Index++)
	{
		if (Hdr.Signature[Index] != Str[Index])
		{
			for (;;){}
		}
	}

	return Hdr;
}

static void PrepareKernelConstructors(const KernelHeader &Header)
{
	DynInfo *KDynInfo = (DynInfo *)(((char*)&kernel_location) + Header.Dynamic);

	/* Run kernel constructors and perform relocation here */
	ApplyRelocations(VirtualAddress, KDynInfo);

	pantheon::vmm::VirtualAddress KVInitArea = VirtualAddress + Header.InitArrayBegin;
	pantheon::vmm::VirtualAddress KVInitEnd = VirtualAddress + Header.InitArrayEnd;
	for (pantheon::vmm::VirtualAddress Cur = KVInitArea; Cur < KVInitEnd; Cur += sizeof(pantheon::vmm::VirtualAddress))
	{
		void (**Func)() = reinterpret_cast<void (**)()>(Cur);
		(*Func)();
	}

	/* TODO: make rodata actually read only */

}

static void PrepareKernelVirtualMemory()
{
	/* For everything in InitBootInfo, map the appropriate physical memory */
	for (UINT64 Index = 0; Index < InitBootInfo.NumMemoryAreas; ++Index)
	{
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

		UINT64 NoExecutePermission = 0;
		NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
		NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN; 
		NoExecutePermission |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
		pantheon::vmm::PageTableEntry NoExecute(Entry);
		NoExecute.SetPagePermissions(NoExecutePermission);

		UINT64 ByteCount = InitBootInfo.InitMemoryAreas[Index].Size - 1;
		ByteCount *= (8 * pantheon::vmm::SmallestPageSize);

		pantheon::vmm::PhysicalAddress StartArea = InitBootInfo.InitMemoryAreas[Index].BaseAddress;
		pantheon::vmm::VirtualAddress BeginPhysicalArea = StartArea + PHYSICAL_MAP_AREA_ADDRESS;
		InitialPageTables.Map(&TTBR1, BeginPhysicalArea, StartArea, ByteCount, NoExecute, FALSE);
	}
}

extern "C" InitialBootInfo *BootInit(fdt_header *dtb)
{
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
		KernelHeader Hdr = ExtractKernelHeader();
		PrepareKernelConstructors(Hdr);

		/* At this point, paging should be set up so the kernel
			* gets the page tables expected...
			*/
		BoardInit(&TTBR1, InitialPageTables);

		volatile bool CheckMe = CheckHeader(dtb);
		if (!CheckMe)
		{
			pantheon::StopError("DTB header invalid", dtb);
		}


		/* The DTB is handled in 3 passes:
			* The first initializes memory areas, so the physical
			* memory manager can be started. The second is to prepare
			* a driver init graph, so the kernel can load drivers as needed.
			* Lastly, another pass is done to simply print the contents of
			* the area.
			*/
		Initialize(dtb);
		InitializeMemory(dtb);
		PrepareKernelVirtualMemory();
	}


	return GetInitBootInfo();
}


/* Necessary for relocation info */
#define R_AARCH64_RELATIVE (1027)


/* This function is defined as part of the kernel, but the actual
 * implementation isn't linked in. This should be cleaned up at some point,
 * but allows most of the relocation code to be moved to somewhere
 * more proper.
 */
extern "C" void ApplyRelocations(UINT64 Base, const DynInfo *DynamicInfo)
{
	enum DynLocation
	{
		REL = 0,
		RELA,
		RELCOUNT,
		RELACOUNT,
		RELENT,
		RELAENT,
		REL_MAX
	};

	UINT64 Items[REL_MAX] = {0};

	for (const auto *Current = DynamicInfo; Current->Tag() != DT_NULL; Current++)
	{
		UINT64 Tag = Current->Tag();
		switch (Tag)
		{
		case DT_REL:
			Items[REL] = Base + Current->Address();
			break;

		case DT_RELA:
			Items[RELA] = Base + Current->Address();
			break;

		case DT_RELENT:
			Items[RELENT] = Current->Value();
			break;

		case DT_RELAENT:
			Items[RELAENT] = Current->Value();
			break;

		case DT_RELCOUNT:
			Items[RELCOUNT] = Current->Value();
			break;

		case DT_RELACOUNT:
			Items[RELACOUNT] = Current->Value();
			break;

		default:
			break;
		}
	}

	for (UINT64 Index = 0; Index < Items[RELCOUNT]; Index++)
	{
		UINT64 EntryAddr = Items[REL] + (Items[RELENT] * Index);
		const RelInfo *Entry = reinterpret_cast<const RelInfo *>(EntryAddr);
		if (Entry && Entry->Type() == R_AARCH64_RELATIVE)
		{
			UINT64 *WriteAddress = reinterpret_cast<UINT64*>(Base + Entry->Address());
			*WriteAddress += Base;
		} 
		else 
		{
			for (;;) {}
		}
	}

	for (UINT64 Index = 0; Index < Items[RELACOUNT]; Index++)
	{
		UINT64 EntryAddr = Items[RELA] + (Items[RELAENT] * Index);
		const RelaInfo *Entry = reinterpret_cast<const RelaInfo *>(EntryAddr);
		if (Entry && Entry->Type() == R_AARCH64_RELATIVE)
		{
			UINT64 *WriteAddress = reinterpret_cast<UINT64*>(Base + Entry->Address());
			*WriteAddress = Base + Entry->Addend();
		}
		else 
		{
			for (;;) {}
		}
	}

}