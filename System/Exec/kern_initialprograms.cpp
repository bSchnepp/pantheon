#include <mmu.hpp>
#include <vmm/vmm.hpp>
#include <vmm/pte.hpp>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/Exec/kern_elf.hpp>
#include <System/Exec/kern_proc_alsr.hpp>
#include <System/Exec/kern_elf_relocations.hpp>
#include <System/Exec/kern_initialprograms.hpp>

#include <System/Proc/kern_sched.hpp>
#include <System/Memory/kern_alloc.hpp>

#ifndef ONLY_TESTS
extern char *sysm_location;
extern char *prgm_location;
#else
const char sysm_location[128] = {};
const char prgm_location[128] = {};
#endif

static void RunElf(ELFFileHeader64 Header, const char *ElfLocation, UINT32 Proc, pantheon::vmm::VirtualAddress BaseAddress)
{
	ELFProgramHeader64 *PrgHeaderTable = (ELFProgramHeader64*)(ElfLocation + Header.e_phoff);

	DynInfo *DynamicSection = nullptr;
	for (UINT64 Index = 0; Index < Header.e_phnum; Index++)
	{
		if (PrgHeaderTable[Index].p_type == PT_DYNAMIC)
		{
			void *Loc = (char*)ElfLocation + PrgHeaderTable[Index].p_offset;
			DynamicSection = reinterpret_cast<DynInfo*>(Loc);
		}
	}

	if (DynamicSection)
	{
		ApplyRelocations(BaseAddress, DynamicSection);
	}

	/* Map in everything and all. */
	for (UINT64 Index = 0; Index < Header.e_phnum; Index++)
	{
		/* What address do we need to load this at? */
		UINT64 BaseVAddr = PrgHeaderTable[Index].p_vaddr + BaseAddress;
		UINT64 CurSize = PrgHeaderTable[Index].p_filesz;

		/* Is this loadable? */
		if (PrgHeaderTable[Index].p_type != PT_LOAD || CurSize == 0)
		{
			continue;
		}

		const char *ProgramLocation = ElfLocation + PrgHeaderTable[Index].p_offset;
		UINT64 NumPages = Align<UINT64>(CurSize, pantheon::vmm::SmallestPageSize) / pantheon::vmm::SmallestPageSize;

		for (UINT64 Count = 0; Count < NumPages; Count++)
		{
			/* Create a new page for every part of the program section... */
			pantheon::vmm::PhysicalAddress NewPage = pantheon::PageAllocator::Alloc();
			pantheon::vmm::VirtualAddress NewPageVirt = pantheon::vmm::PhysicalToVirtualAddress(NewPage);

			const char *FinalLocation = (ProgramLocation + (pantheon::vmm::SmallestPageSize * Count));
			UINT64 TargetSize = (CurSize > pantheon::vmm::SmallestPageSize) ? pantheon::vmm::SmallestPageSize : CurSize;
			
			CopyMemory((void*)NewPageVirt, (void*)FinalLocation, TargetSize);
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

			pantheon::Scheduler::MapPages(Proc, &TargetVAddr, &NewPage, UEntry, 1);
		}
	}
}

static void RunExecutable(void *ElfLocation, const char *Name)
{
	Optional<ELFFileHeader64> Header = pantheon::exec::ParseElfFileHeader(ElfLocation);
	if (Header.GetOkay() == FALSE)
	{
		pantheon::StopErrorFmt("unable to unpack %s\n", Name);
	}
	/* Okay, now make sure these are actually executables... */
	if (Header().e_type != ET_DYN && Header().e_type != ET_EXEC)
	{
		pantheon::StopErrorFmt("%s not an executable\n", Name);
	}

	pantheon::vmm::VirtualAddress Base = pantheon::GenerateALSRBase();
	UINT32 PID = pantheon::Scheduler::CreateProcess(Name, (void*)(Base + Header().e_entry));
	RunElf(Header(), (const char*)ElfLocation, PID, Base);
	pantheon::Scheduler::SetState(PID, pantheon::Process::STATE_RUNNING);	

	/* Create our initial thread */
	pantheon::Scheduler::CreateThread(PID, (void*)(Base + Header().e_entry), nullptr);
}

void pantheon::UnpackInitPrograms()
{
	RunExecutable((void*)&sysm_location, "sysm");
	RunExecutable((void*)&prgm_location, "prgm");
}