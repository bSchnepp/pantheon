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

static void RunElf(ELFFileHeader64 Header, const char *ElfLocation, pantheon::Process *Proc)
{
	Proc->Lock();

	ELFProgramHeader64 *PrgHeaderTable = (ELFProgramHeader64*)(ElfLocation + Header.e_phoff);
	UINT64 NumPrg = Header.e_phnum;
	
	/* Map in everything and all. */
	for (UINT64 Index = 0; Index < NumPrg; Index++)
	{
		if (PrgHeaderTable[Index].p_type == 0)
		{
			continue;
		}

		UINT64 BaseVAddr = PrgHeaderTable[Index].p_vaddr;
		UINT64 CurSize = PrgHeaderTable[Index].p_filesz;

		if (CurSize == 0)
		{
			continue;
		}

		UINT64 NumPages = Align<UINT64>(CurSize, pantheon::vmm::SmallestPageSize);
		const char *ProgramLocation = ElfLocation + PrgHeaderTable[Index].p_offset;
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

			/* Let's go ahead and map in everything in the lower table as-is. */
			pantheon::vmm::PageTableEntry UEntry;
			UEntry.SetBlock(TRUE);
			UEntry.SetMapped(TRUE);
			UEntry.SetPagePermissions(0b01 << 6);
			UEntry.SetKernelNoExecute(TRUE);
			UEntry.SetUserNoExecute(FALSE);
			UEntry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
			UEntry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
			UEntry.SetMAIREntry(pantheon::vmm::MAIREntry_1);
			Proc->MapPages(&TargetVAddr, &NewPage, UEntry, 1);
		}
	}
	/* Set the process to running here... 
	 * Until we properly have a higher half kernel, 
	 * this is unsafe, since we could change memory from under the kernel
	 * while it's still in use. */
	Proc->SetState(pantheon::PROCESS_STATE_RUNNING);
	Proc->Unlock();
}

static void RunSysm(ELFFileHeader64 Header, const char *ElfLocation)
{
	pantheon::Process *sysm = pantheon::GetGlobalScheduler()->CreateProcess("sysm", (void*)Header.e_entry);
	RunElf(Header, ElfLocation, sysm);
}

static void RunPrgm(ELFFileHeader64 Header, const char *ElfLocation)
{
	pantheon::Process *prgm = pantheon::GetGlobalScheduler()->CreateProcess("prgm", (void*)Header.e_entry);
	RunElf(Header, ElfLocation, prgm);
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