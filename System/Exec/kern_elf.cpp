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
	constexpr const unsigned char MagicData[] = {0x7F, 'E', 'L', 'F'};
	for (UINT8 Index = 0; Index < 4; Index++)
	{
		if (FinalHeader.e_ident[Index + 0] != MagicData[Index])
		{
			return Optional<ELFFileHeader64>();	
		}
	}

	/* One restriction: everything must be 64-bit. 
	 * 1 is 32-bit, and 2 is 64-bit.
	 */
	if (FinalHeader.e_ident[EI_CLASS] != 2)
	{
		return Optional<ELFFileHeader64>();
	}

	/* Make sure it's ABI compatible with ELF version 1. */
	if (FinalHeader.e_ident[EI_VERSION] != 1)
	{
		return Optional<ELFFileHeader64>();
	}

	/* We don't bother with checking endianness, 
	 * header version, etc, architecture, etc. 
	 * Since these images we're loading here are implied to be valid:
	 * (ie, prgm should load sections and submit them for a CreateProcess 
	 * job itself), we should give some trust to these initial programs.
	 */
	return Optional<ELFFileHeader64>(FinalHeader);
}


pantheon::exec::ElfParser::ElfParser(void *Data) : Data(Data){}

pantheon::exec::ElfParser::~ElfParser() = default;

Optional<ELFFileHeader64> pantheon::exec::ElfParser::ParseHeader()
{
	/* 
	 * TODO: Absorb this function, and remove any uses of it. Force
	 * use of this class instead! 
	 */
	Optional<ELFFileHeader64> Result = pantheon::exec::ParseElfFileHeader(this->Data);
	if (Result.GetOkay())
	{
		return Result;
	}
	return Result;
}

Optional<ELFSectionHeader64> pantheon::exec::ElfParser::ParseSectionHeader()
{
	/* Check if we already parsed the header. 
	 * If not, go ahead and try to do it again.
	 */
	if (!this->FileHeader.GetOkay())
	{
		this->ParseHeader();
	}

	/* If it's still bad, go give up. */
	if (!this->FileHeader.GetOkay())
	{
		return Optional<ELFSectionHeader64>();
	}

	char *AsChar = (char*)this->Data;
	ELFSectionHeader64 *Header = (ELFSectionHeader64*)(AsChar + this->FileHeader.GetValue().e_shoff);

	ELFSectionHeader64 Result;
	CopyMemory((VOID*)&Result, (VOID*)Header, sizeof(ELFSectionHeader64));
	return Optional<ELFSectionHeader64>(Result);
}

Optional<ELFProgramHeader64> pantheon::exec::ElfParser::ParseProgramHeader()
{
	/* Check if we already parsed the header. 
	 * If not, go ahead and try to do it again.
	 */
	if (!this->FileHeader.GetOkay())
	{
		this->ParseHeader();
	}

	/* If it's still bad, go give up. */
	if (!this->FileHeader.GetOkay())
	{
		return Optional<ELFProgramHeader64>();
	}

	char *AsChar = (char*)this->Data;
	ELFProgramHeader64 *Header = (ELFProgramHeader64*)(AsChar + this->FileHeader.GetValue().e_phoff);

	ELFProgramHeader64 Result;
	CopyMemory((VOID*)&Result, (VOID*)Header, sizeof(ELFProgramHeader64));
	return Optional<ELFProgramHeader64>(Result);	
}