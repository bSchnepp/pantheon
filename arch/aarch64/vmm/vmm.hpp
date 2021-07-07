#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

namespace pantheon::vmm
{

typedef enum PageGranularity : UINT64
{
	PAGE_GRANULARITY_PAGE = 0b11,
	PAGE_GRANULARITY_BLOCK = 0b01,
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

typedef UINT64 PageTableEntry;
typedef PageTableEntry PageTable[512];

PageTableEntry CreateEntry(const PageTableEntry *NextLevel, PageGranularity Size, PageAccessor Accessor, UINT64 Permission, PageSharableType Sharable, PageTypeMMIOAccessor MMIOType);

PageTable *CreateBasicPageTables();

}

#endif