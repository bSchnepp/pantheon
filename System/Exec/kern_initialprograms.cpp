#include <mmu.hpp>
#include <vmm/vmm.hpp>
#include <vmm/pte.hpp>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/Exec/kern_elf.hpp>
#include <System/Exec/kern_initialprograms.hpp>

#include <System/Proc/kern_sched.hpp>
#include <System/PhyMemory/kern_alloc.hpp>

extern char *sysm_location;
extern char *prgm_location;

static void RunElf(ELFFileHeader64 Header, const char *ElfLocation, UINT32 Proc)
{
	ELFProgramHeader64 *PrgHeaderTable = (ELFProgramHeader64*)(ElfLocation + Header.e_phoff);
	UINT64 NumPrg = Header.e_phnum;
	
	/* Map in everything and all. */
	for (UINT64 Index = 0; Index < NumPrg; Index++)
	{
		/* Is this loadable? */
		if (PrgHeaderTable[Index].p_type != PT_LOAD)
		{
			continue;
		}

		/* What address do we need to load this at? */
		UINT64 BaseVAddr = PrgHeaderTable[Index].p_vaddr;
		UINT64 CurSize = PrgHeaderTable[Index].p_filesz;

		/* If this program section is really empty, don't bother doing anything. */
		if (CurSize == 0)
		{
			continue;
		}

		const char *ProgramLocation = ElfLocation + PrgHeaderTable[Index].p_offset;
		UINT64 NumPages = Align<UINT64>(CurSize, pantheon::vmm::SmallestPageSize);
		for (UINT64 Count = 0; Count < NumPages / pantheon::vmm::SmallestPageSize; Count++)
		{
			/* Create a new page for every part of the program section... */
			pantheon::vmm::PhysicalAddress NewPage = pantheon::PageAllocator::Alloc();
			/* TODO: virtualize this address for kernel space! */

			const char *FinalLocation = (ProgramLocation + (pantheon::vmm::SmallestPageSize * Count));
			UINT64 TargetSize = (CurSize > pantheon::vmm::SmallestPageSize) ? pantheon::vmm::SmallestPageSize : CurSize;
			
			/* Clear the page first before we use it. */
			SetBufferBytes((CHAR*)NewPage, 0x00, pantheon::vmm::SmallestPageSize);
			CopyMemory((void*)NewPage, (void*)FinalLocation, TargetSize);
			CurSize -= TargetSize;

			/* Map this page in now... */
			UINT64 TargetVAddr = BaseVAddr + (pantheon::vmm::SmallestPageSize * Count);

			/* Go ahead and make the section in */
			pantheon::vmm::PageTableEntry UEntry;
			UEntry.SetBlock(TRUE);
			UEntry.SetMapped(TRUE);
			UEntry.SetPagePermissions(0b11 << 6);
			UEntry.SetKernelNoExecute(TRUE);
			UEntry.SetUserNoExecute(FALSE);
			UEntry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
			UEntry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
			UEntry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

			pantheon::GetGlobalScheduler()->MapPages(Proc, &TargetVAddr, &NewPage, UEntry, 1);
		}
	}
	pantheon::GetGlobalScheduler()->SetState(Proc, pantheon::PROCESS_STATE_RUNNING);
}

static void RunSysm(ELFFileHeader64 Header, const char *ElfLocation)
{
	UINT32 PID = pantheon::GetGlobalScheduler()->CreateProcess("sysm", (void*)Header.e_entry);
	RunElf(Header, ElfLocation, PID);
}

static void RunPrgm(ELFFileHeader64 Header, const char *ElfLocation)
{
	UINT32 PID = pantheon::GetGlobalScheduler()->CreateProcess("prgm", (void*)Header.e_entry);
	RunElf(Header, ElfLocation, PID);
}

void pantheon::UnpackInitPrograms()
{
	Optional<ELFFileHeader64> SysmHeader = pantheon::exec::ParseElfFileHeader((void*)&sysm_location);
	Optional<ELFFileHeader64> PrgmHeader = pantheon::exec::ParseElfFileHeader((void*)&prgm_location);

	if (SysmHeader.GetOkay() == FALSE || PrgmHeader.GetOkay() == FALSE)
	{
		pantheon::StopError("unable to unpack initial programs");
	}

	/* Okay, now make sure these are actually executables... */
	if (SysmHeader().e_type != ET_REL && SysmHeader().e_type != ET_EXEC)
	{
		pantheon::StopError("sysm not an executable");
	}

	if (PrgmHeader().e_type != ET_REL && PrgmHeader().e_type != ET_EXEC)
	{
		pantheon::StopError("prgm not an executable");
	}

	RunSysm(SysmHeader(), (char*)&sysm_location);
	RunPrgm(PrgmHeader(), (char*)&prgm_location);
}