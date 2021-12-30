#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/Exec/kern_elf.hpp>
#include <System/Exec/kern_initialprograms.hpp>


extern char *sysm_location;
extern char *prgm_location;

void pantheon::UnpackInitPrograms()
{
	Optional<ELFFileHeader64> SysmHeader = pantheon::exec::ParseElfFileHeader((void*)&sysm_location);
	Optional<ELFFileHeader64> PrgmHeader = pantheon::exec::ParseElfFileHeader((void*)&prgm_location);

	if (SysmHeader.GetOkay() == FALSE || PrgmHeader.GetOkay() == FALSE)
	{
		pantheon::StopError("unable to unpack initial programs");
	}
}