#include <kern_datatypes.hpp>
#include <Common/Structures/kern_optional.hpp>

#ifndef _PANTHEON_KERN_ELF_H_
#define _PANTHEON_KERN_ELF_H_

constexpr UINT8 NUM_IDENT = 16;

constexpr UINT8 EI_MAG0 = 0;
constexpr UINT8 EI_MAG1 = 1;
constexpr UINT8 EI_MAG2 = 2;
constexpr UINT8 EI_MAG3 = 3;

constexpr UINT8 EI_CLASS = 4;
constexpr UINT8 EI_DATA = 5;
constexpr UINT8 EI_VERSION = 6;
constexpr UINT8 EI_OSABI = 7;
constexpr UINT8 EI_OSABIVERSION = 8;
constexpr UINT8 EI_PAD = 9;

enum FileHeaderEType : UINT16
{
	ET_NONE = 0x00,
	ET_REL,
	ET_EXEC,
	ET_DYN,
	ET_CORE,
	ET_LOOS = 0xFE00,
	ET_HIOS = 0xFEFF,
	ET_LOPROC = 0xFF00,
	ET_HIPROC = 0xFFFF,	
};

enum FileHeaderEMachine : UINT16
{
	EM_NONE = 0x00,
	EM_I386 = 0x03,
	EM_PPC32 = 0x14,
	EM_PPC64 = 0x15,
	EM_ARMv7 = 0x28,
	EM_X8664 = 0x3E,
	EM_AARCH64 = 0xB7,
	EM_RISCV = 0xF3,
};

struct ELFFileHeader64
{
	UINT8 e_ident[NUM_IDENT];
	UINT16 e_type;
	UINT16 e_machine;
	UINT32 e_version;
	UINT64 e_entry;
	UINT64 e_phoff;
	UINT64 e_shoff;
	UINT32 e_flags;
	UINT16 e_ehsize;
	UINT16 e_phentsize;
	UINT16 e_phnum;
	UINT16 e_shentsize;
	UINT16 e_shnum;
	UINT16 e_shstrndx;
}__attribute__((__packed__));

static_assert(sizeof(ELFFileHeader64) == 0x40);

enum ProgramHeaderPType : UINT32
{
	PT_NULL = (0x00),
	PT_LOAD = (0x01),
	PT_DYNAMIC = (0x02),
	PT_INTERP = (0x03),
	PT_NOTE = (0x04),
	PT_SHLIB = (0x05),
	PT_PHDR = (0x06),
	PT_LOOS = (0x60000000),
	PT_HIOS = (0x6FFFFFFF),
	PT_LOPROC = (0x70000000),
	PT_HIPROC = (0x7FFFFFFF),
};

struct ELFProgramHeader64
{
	UINT32 p_type;
	UINT32 p_flags;
	UINT64 p_offset;
	UINT64 p_vaddr;
	UINT64 p_paddr;
	UINT64 p_filesz;
	UINT64 p_memsz;
	UINT64 p_align;
}__attribute__((__packed__));

struct ELFSectionHeader64
{
	UINT32 sh_name;
	UINT32 sh_type;

	UINT64 sh_flags;

	UINT64 sh_addr;
	UINT64 sh_offset;
	UINT64 sh_size;

	UINT32 sh_link;
	UINT32 sh_info;

	UINT64 sh_addralign;
	UINT64 sh_entsize;
}__attribute__((__packed__));

namespace pantheon::exec
{

Optional<ELFFileHeader64> ParseElfFileHeader(void *Data);

}

#endif
