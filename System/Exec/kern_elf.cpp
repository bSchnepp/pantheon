#include <kern_datatypes.hpp>
#include <System/Exec/kern_elf.hpp>
#include <Common/Structures/kern_optional.hpp>

#include <kern_runtime.hpp>

Optional<ELFFileHeader64> pantheon::exec::ParseElfFileHeader(void *Data)
{
	/* Assume the pointer is valid... */
	ELFFileHeader64 FinalHeader;
	CopyMemory(&FinalHeader, Data, sizeof(ELFFileHeader64));

	/* Now, make sure the data makes sense. */
	constexpr const char MagicData[] = {0x7F, 'E', 'L', 'F'};
	for (UINT8 Index = 0; Index < 4; Index++)
	{
		if (FinalHeader.e_ident[Index + 0] != MagicData[Index])
		{
			return Optional<ELFFileHeader64>();	
		}
	}

	/* We don't bother with checking endianness, 
	 * header version, etc, architecture, etc. 
	 * Since these images we're loading here are implied to be valid:
	 * (ie, prgm should load sections and submit them for a CreateProcess 
	 * job itself), we should give some trust to these initial programs.
	 */
	return Optional<ELFFileHeader64>(FinalHeader);
}