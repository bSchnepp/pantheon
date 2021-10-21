#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <Common/Structures/kern_slab.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

namespace pantheon::vmm
{

namespace BlockSize
{
	constexpr UINT64 L1BlockSize = (512ULL * 1024ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L2BlockSize = (1ULL * 1024ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L3BlockSize = (2ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L4BlockSize = (4ULL * 1024ULL);
}

typedef enum PageAccessor : UINT64
{
	PAGE_ACCESSOR_KERNEL = (0b00ULL << 6),
	PAGE_ACCESSOR_USER = (0b01ULL << 6),
}PageAccessor;

typedef enum PagePermissionRaw : UINT64
{
	PAGE_PERMISSION_READ_ONLY_KERN = (0b1ULL << 6),
	PAGE_PERMISSION_READ_WRITE_KERN = (0b0ULL << 6),

	PAGE_PERMISSION_READ_ONLY_USER = (0b1ULL << 7),
	PAGE_PERMISSION_READ_WRITE_USER = (0b0ULL << 7),

	PAGE_PERMISSION_EXECUTE_USER = (0ULL << 54),
	PAGE_PERMISSION_EXECUTE_KERN = (0ULL << 53),

	PAGE_PERMISSION_NO_EXECUTE_USER = (1ULL << 54),
	PAGE_PERMISSION_NO_EXECUTE_KERN = (1ULL << 53),
}PagePermissionRaw;

typedef enum PagePermission : UINT64
{
	PAGE_PERMISSION_KERNEL_RWX = PAGE_PERMISSION_READ_WRITE_KERN | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_RW = PAGE_PERMISSION_READ_WRITE_KERN | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_NO_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_RX = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_R = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_NO_EXECUTE_KERN,

	PAGE_PERMISSION_USER_RW = PAGE_PERMISSION_READ_WRITE_KERN | PAGE_PERMISSION_READ_WRITE_USER | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_NO_EXECUTE_KERN,
	PAGE_PERMISSION_USER_RX = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_EXECUTE_USER | PAGE_PERMISSION_NO_EXECUTE_KERN,
	PAGE_PERMISSION_USER_R = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_NO_EXECUTE_USER | PAGE_PERMISSION_NO_EXECUTE_KERN,
}PagePermission;

typedef enum PageAccessed : UINT64
{
	PAGE_MISC_NOT_ACCESSED = (0ULL << 10),
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
	PAGE_TYPE_MMIO_ACCESSOR_NORMAL_NO_CACHE = (0b10ULL << 2),
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

typedef UINT64 PageTableEntryRaw;
typedef UINT64 PhysicalAddress;
typedef UINT64 VirtualAddress;
/**
 * \~english @brief Describes a page table entry, assuming 4KB pages.
 * \~english @author Brian Schnepp
 */
class PageTableEntry
{
public:
	constexpr explicit PageTableEntry() noexcept : Raw(0) {}
	constexpr explicit PageTableEntry(PageTableEntry &&Other) noexcept : Raw(Other.Raw){}
	constexpr explicit PageTableEntry(const PageTableEntry &Other) noexcept : Raw(Other.Raw){}
	constexpr explicit PageTableEntry(const PageTableEntry &Other, UINT64 Attr) noexcept : Raw(Other.Raw | Attr){}
	~PageTableEntry(){};

	[[nodiscard]] constexpr BOOL IsMapped() const { return this->GetBits(0, 1) != 0; }
	[[nodiscard]] constexpr BOOL IsBlock() const { return this->GetBits(1, 1) == 0; }
	[[nodiscard]] constexpr BOOL IsTable() const { return this->GetBits(1, 1) != 0; }
	[[nodiscard]] constexpr MAIREntry GetMAIREntry() const { return static_cast<MAIREntry>(this->GetMaskedBits(2, 3)); };
	[[nodiscard]] constexpr BOOL IsNonSecure() const { return this->GetBits(5, 1) != 0; }
	[[nodiscard]] constexpr BOOL IsUserAccessible() const { return this->GetBits(6, 1) != 0; }
	[[nodiscard]] constexpr BOOL IsUserReadOnly() const { return this->GetBits(7, 1) != 0; }
	[[nodiscard]] constexpr PageSharableType GetSharable() const { return static_cast<PageSharableType>(this->GetMaskedBits(8, 2)); };
	[[nodiscard]] constexpr PageAccessed GetAccessor() const { return static_cast<PageAccessed>(this->GetMaskedBits(10, 1)); };
	[[nodiscard]] constexpr BOOL IsKernelNoExecute() const { return this->GetBits(53, 1) != 0; };
	[[nodiscard]] constexpr BOOL IsUserNoExecute() const { return this->GetBits(54, 1) != 0; };
	[[nodiscard]] constexpr PhysicalAddress GetPhysicalAddressArea() const { return static_cast<PhysicalAddress>(this->GetMaskedBits(12, 48)); };
	[[nodiscard]] constexpr UINT64 GetRawAttributes() const { return this->Raw; }
	[[nodiscard]] constexpr PagePermissionRaw GetReadPermission() { return static_cast<PagePermissionRaw>(this->GetMaskedBits(6, 2)); }
	[[nodiscard]] constexpr PagePermissionRaw GetWritePermission() { return static_cast<PagePermissionRaw>(this->GetMaskedBits(6, 2)); }

	constexpr VOID SetRawAttributes(UINT64 Value) { this->Raw = Value; };
	constexpr PageTableEntry &operator=(const PageTableEntry &Other) 
	{ 
		if (&Other != this) 
		{ 
			this->Raw = Other.Raw;
		}
		return *this;
	}

	constexpr VOID SetMapped(BOOL Value) { this->SetBits(0, 1, Value == 1); }
	constexpr VOID SetBlock(BOOL Value) { this->SetBits(1, 1, Value == 0); }
	constexpr VOID SetTable(BOOL Value) { this->SetBits(1, 1, Value == 1); }
	constexpr VOID SetMAIREntry(MAIREntry Value) { this->SetBitsRaw(2, 3, Value); }
	constexpr VOID SetNonSecure(BOOL Value) { this->SetBits(5, 1, Value != 0); }
	constexpr VOID SetUserAccessible(BOOL Value) { this->SetBits(6, 1, Value != 0); }
	constexpr VOID SetUserReadOnly(BOOL Value) { this->SetBits(7, 1, Value != 0); }
	constexpr VOID SetSharable(PageSharableType Value) { this->SetBitsRaw(8, 2, Value); }
	constexpr VOID SetAccessor(PageAccessed Value) { this->SetBitsRaw(10, 1, Value); };
	constexpr VOID SetKernelNoExecute(BOOL Value) { this->SetBits(53, 1, Value != 0); };
	constexpr VOID SetUserNoExecute(BOOL Value) { this->SetBits(54, 1, Value != 0); };
	constexpr VOID SetPagePermissions(PagePermission Val) { this->Raw &= ~PAGE_PERMISSION_KERNEL_RWX | PAGE_PERMISSION_USER_RW | PAGE_PERMISSION_USER_RX; this->Raw |= Val; }
	constexpr VOID SetPhysicalAddressArea(PhysicalAddress Addr) { this->Raw |= Addr & 0xFFFFFFFFFFFF000; }

protected:
	[[nodiscard]] 
	constexpr UINT64 GetBits(UINT64 Offset, UINT64 Count) const
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + (Offset > 64) * (64);
		Count = ((Count <= 64) * (Count)) + (Count > 64) * (64);
		UINT64 Mask = (1ULL << Count) - 1;
		UINT64 Shifted = (this->Raw >> Offset);
		return Shifted & Mask;
	}

	[[nodiscard]] 
	constexpr UINT64 GetMaskedBits(UINT64 Offset, UINT64 Count) const
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + (Offset > 64) * (64);
		Count = ((Count <= 64) * (Count)) + (Count > 64) * (64);
		UINT64 Mask = (1ULL << Count) - 1;
		return (this->Raw) & ((Mask << Offset) & Mask);
	}

	constexpr VOID SetBits(UINT64 Offset, UINT64 Count, UINT64 Value)
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + (Offset > 64) * (64);
		Count = ((Count <= 64) * (Count)) + (Count > 64) * (64);
		const UINT64 Mask = ((1ULL << Count) - 1);
		this->Raw &= ~(Mask << Offset);
		Value &= Mask;
		this->Raw |= Value << Offset;
	}

	constexpr VOID SetBitsRaw(UINT64 Offset, UINT64 Count, UINT64 Value)
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + (Offset > 64) * (64);
		Count = ((Count <= 64) * (Count)) + (Count > 64) * (64);
		const UINT64 Mask = ((1ULL << Count) - 1);
		this->Raw &= ~(Mask << Offset);
		this->Raw |= (Value & (Mask << Offset));
	}

private:
	PageTableEntryRaw Raw;
};

typedef struct PageTable
{
	pantheon::vmm::PageTableEntry Entries[512];
}PageTable;
static_assert(sizeof(PageTable) == 4096);

inline VirtualAddress PhysicalToVirtualAddress(PhysicalAddress PhyAddr)
{
	return PhyAddr | (0b1111111111111111ULL << 48);
}

inline PhysicalAddress VirtualToPhysicalAddress(PageTable *RootTable, VirtualAddress Addr)
{
	/* TODO: traverse page table */
	PANTHEON_UNUSED(RootTable);
	return Addr;
}

VOID InvalidateTLB();
VOID EnablePaging();

static_assert(sizeof(PageTableEntry) == sizeof(PageTableEntryRaw));

class PageAllocator
{
public:
	PageAllocator(VOID *Area, UINT64 Pages)
	{
		this->Allocator = pantheon::mm::SlabCache<pantheon::vmm::PageTable>(Area, Pages);
	}
	
	~PageAllocator()
	{

	}

	pantheon::vmm::PageTable *Allocate()
	{
		return this->Allocator.Allocate();
	}

	[[nodiscard]] UINT64 SpaceLeft() const
	{
		return this->Allocator.SpaceLeft();
	}

	[[nodiscard]] UINT64 SpaceUsed() const
	{
		return this->Allocator.SlabCount() - this->SpaceLeft();
	}

	void Map(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions);

private:
	pantheon::mm::SlabCache<pantheon::vmm::PageTable> Allocator;
};

}

extern "C" VOID write_ttbr0_el1(UINT64 Val);
extern "C" VOID write_ttbr1_el1(UINT64 Val);

#endif