#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

namespace pantheon::vmm
{

typedef enum PageGranularity : UINT64
{
	PAGE_GRANULARITY_PAGE = 0b11ULL,
	PAGE_GRANULARITY_BLOCK = 0b01ULL,
}PageGranularity;

typedef enum PageAccessor : UINT64
{
	PAGE_ACCESSOR_KERNEL = (0b00ULL << 6),
	PAGE_ACCESSOR_USER = (0b01ULL << 6),
}PageAccessor;

typedef enum PagePermission : UINT64
{
	PAGE_PERMISSION_READ_ONLY_KERN = (0b1ULL << 6),
	PAGE_PERMISSION_READ_WRITE_KERN = (0b0ULL << 6),

	PAGE_PERMISSION_READ_ONLY_USER = (0b1ULL << 7),
	PAGE_PERMISSION_READ_WRITE_USER = (0b0ULL << 7),

	PAGE_PERMISSION_NO_EXECUTE_USER = (1ULL << 54),
	PAGE_PERMISSION_NO_EXECUTE_KERN = (1ULL << 53),
}PagePermission;

typedef enum PageMisc : UINT64
{
	PAGE_MISC_ACCESSED = (1ULL << 10),
}PageMisc;

typedef enum PageSharableType : UINT64
{
	PAGE_SHARABLE_TYPE_NONE = (0b00ULL << 8),
	PAGE_SHARABLE_TYPE_OUTER = (0b10ULL << 8),
	PAGE_SHARABLE_TYPE_INNER = (0b11ULL << 8),
}PageSharableType;

typedef enum PageTypeMMIOAccessor : UINT64
{
	PAGE_TYPE_MMIO_ACCESSOR_NORMAL = (0b00ULL << 2),
	PAGE_TYPE_MMIO_ACCESSOR_DEVICE = (0b01ULL << 2),
	PAGE_TYPE_MMIO_ACCESSOR_NO_CACHE = (0b10ULL << 2),
}PageTypeMMIOAccessor;

typedef enum MAIREntry
{
	MAIREntry_0 = (0b000ULL << 2),
	MAIREntry_1 = (0b001ULL << 2),
	MAIREntry_2 = (0b010ULL << 2),
	MAIREntry_3 = (0b011ULL << 2),
	MAIREntry_4 = (0b100ULL << 2),
	MAIREntry_5 = (0b101ULL << 2),
	MAIREntry_6 = (0b110ULL << 2),
	MAIREntry_7 = (0b111ULL << 2),
}MAIREntry;

typedef UINT64 PageTableEntry;

PageTableEntry CreateEntry(const PageTableEntry *NextLevel, PageGranularity Size, PageAccessor Accessor, UINT64 Permission, PageSharableType Sharable, PageTypeMMIOAccessor MMIOType);
PageTableEntry CreateEntry(const PageTableEntry *NextLevel, UINT64 Attributes);

constexpr UINT64 IdentityAttributes()
{
	UINT64 IdentityFlags = 0;
	IdentityFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	IdentityFlags |= pantheon::vmm::PAGE_ACCESSOR_KERNEL;
	IdentityFlags |= pantheon::vmm::PAGE_SHARABLE_TYPE_INNER;
	IdentityFlags |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	IdentityFlags |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_USER;
	IdentityFlags |= pantheon::vmm::PAGE_GRANULARITY_PAGE;
	IdentityFlags |= pantheon::vmm::MAIREntry_0;
	return IdentityFlags;
}

constexpr UINT64 TextAttributes()
{
	UINT64 TextFlags = 0;
	TextFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	TextFlags |= pantheon::vmm::PAGE_ACCESSOR_KERNEL;
	TextFlags |= pantheon::vmm::PAGE_SHARABLE_TYPE_INNER;
	TextFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_KERN;
	TextFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	TextFlags |= pantheon::vmm::PAGE_GRANULARITY_PAGE;
	TextFlags |= pantheon::vmm::MAIREntry_0;
	return TextFlags;
}

constexpr UINT64 RodataAttributes()
{
	UINT64 RodataFlags = 0;
	RodataFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	RodataFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN;
	RodataFlags |= pantheon::vmm::PAGE_ACCESSOR_KERNEL;
	RodataFlags |= pantheon::vmm::PAGE_SHARABLE_TYPE_INNER;
	RodataFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_KERN;
	RodataFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	RodataFlags |= pantheon::vmm::PAGE_GRANULARITY_PAGE;
	RodataFlags |= pantheon::vmm::MAIREntry_0;
	return RodataFlags;
}

constexpr UINT64 DataAttributes()
{
	UINT64 DataFlags = 0;
	DataFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_USER;
	DataFlags |= pantheon::vmm::PAGE_PERMISSION_NO_EXECUTE_KERN;
	DataFlags |= pantheon::vmm::PAGE_ACCESSOR_KERNEL;
	DataFlags |= pantheon::vmm::PAGE_SHARABLE_TYPE_INNER;
	DataFlags |= pantheon::vmm::PAGE_PERMISSION_READ_ONLY_USER;
	DataFlags |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	DataFlags |= pantheon::vmm::PAGE_GRANULARITY_PAGE;
	DataFlags |= pantheon::vmm::MAIREntry_0;
	return DataFlags;
}

}

extern "C" VOID write_ttbr0_el1(UINT64 Val);
extern "C" VOID write_ttbr1_el1(UINT64 Val);

#endif