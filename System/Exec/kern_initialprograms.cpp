#include <mmu.hpp>
#include <vmm/vmm.hpp>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/Exec/kern_elf.hpp>
#include <System/Exec/kern_initialprograms.hpp>

#include <System/Proc/kern_sched.hpp>

extern char *sysm_location;
extern char *prgm_location;

static void RunSysm(UINT64 IP)
{
	pantheon::GetGlobalScheduler()->CreateProcess("sysm.elf", (void*)IP);	
}

static void RunPrgm(UINT64 IP)
{
	pantheon::GetGlobalScheduler()->CreateProcess("prgm.elf", (void*)IP);
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

	RunSysm(SysmHeader().e_entry);
	RunPrgm(PrgmHeader().e_entry);
}