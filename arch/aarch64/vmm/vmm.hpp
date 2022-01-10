#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/sync.hpp>
#include <Common/Structures/kern_slab.hpp>

#ifndef _AARCH64_VIRT_MEM_HPP_
#define _AARCH64_VIRT_MEM_HPP_

namespace pantheon::vmm
{

namespace BlockSize
{
	constexpr UINT64 L0BlockSize = (512ULL * 1024ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L1BlockSize = (1ULL * 1024ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L2BlockSize = (2ULL * 1024ULL * 1024ULL);
	constexpr UINT64 L3BlockSize = (4 * 1024ULL);
}

constexpr UINT64 SmallestPageSize = pantheon::vmm::BlockSize::L3BlockSize;

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
	PAGE_PERMISSION_KERNEL_RWX = PAGE_PERMISSION_READ_WRITE_KERN | PAGE_PERMISSION_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_RW = PAGE_PERMISSION_READ_WRITE_KERN  | PAGE_PERMISSION_NO_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_RX = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_EXECUTE_KERN,
	PAGE_PERMISSION_KERNEL_R = PAGE_PERMISSION_READ_ONLY_KERN | PAGE_PERMISSION_NO_EXECUTE_KERN,

	PAGE_PERMISSION_USER_RW = PAGE_PERMISSION_READ_WRITE_USER | PAGE_PERMISSION_NO_EXECUTE_USER,
	PAGE_PERMISSION_USER_RX = PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_EXECUTE_USER,
	PAGE_PERMISSION_USER_R = PAGE_PERMISSION_READ_ONLY_USER | PAGE_PERMISSION_NO_EXECUTE_USER,
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

typedef enum SCTLRFlags : UINT64
{
	SCTLR_M = (1ULL << 0),
	SCTLR_A = (1ULL << 1),
	SCTLR_C = (1ULL << 2),
	SCTLR_SA = (1ULL << 3),
	SCTLR_SA0 = (1ULL << 4),
	SCTLR_CP15BEN = (1ULL << 5),
	SCTLR_nAA = (1ULL << 6),
	SCTLR_ITD = (1ULL << 7),
	SCTLR_SED = (1ULL << 8),
	SCTLR_UMA = (1ULL << 9),
	SCTLR_EnRCTX = (1ULL << 10),
	SCTLR_EOS = (1ULL << 11),
	SCTLR_I = (1ULL << 12),
	SCTLR_EnDB = (1ULL << 13),
	SCTLR_DZE = (1ULL << 14),
	SCTLR_UCT = (1ULL << 15),
	SCTLR_nTWI = (1ULL << 16),
	SCTLR_RES0 = (1ULL << 17),
	SCTLR_nTWE = (1ULL << 18),
	SCTLR_WXN = (1ULL << 19),
	SCTLR_TSCXT = (1ULL << 20),
	SCTLR_IESB = (1ULL << 21),
	SCTLR_EIS = (1ULL << 22),
	SCTLR_SPAN = (1ULL << 23),
	SCTLR_E0E = (1ULL << 24),
	SCTLR_EE = (1ULL << 25),
	SCTLR_UCI = (1ULL << 26),
	SCTLR_EnDA = (1ULL << 27),
	SCTLR_nTLSMD = (1ULL << 28),
	SCTLR_LSMAOE = (1ULL << 29),
	SCTLR_EnIB = (1ULL << 30),
	SCTLR_EnIA = (1ULL << 31),
	SCTLR_CMOW = (1ULL << 32),
	SCTLR_MSCEn = (1ULL << 33),
	SCTLR_RES1 = (1ULL << 34),
	SCTLR_BT0 = (1ULL << 35),
	SCTLR_BT1 = (1ULL << 36),
	SCTLR_ITFSB = (1ULL << 37),
	SCTLR_ATA0 = (1ULL << 42),
	SCTLR_ATA = (1ULL << 43),
	SCTLR_DSSBS = (1ULL << 44),
	SCTLR_TWEDEn = (1ULL << 45),
	SCTLR_EnASR = (1ULL << 54),
	SCTLR_EnAS0 = (1ULL << 55),
	SCTLR_EnALS = (1ULL << 56),
	SCTLR_EPAN = (1ULL << 57),
	SCTLR_NMI = (1ULL << 61),
	SCTLR_SPINTMASK = (1ULL << 62),
	SCTLR_TIDCP = (1ULL << 63),
}SCTLRFlags;

typedef enum SCTLRBitfields : UINT64
{
	SCTLR_TAG_CHECK_FAULT_EL0_IGNORE = (0b00ULL << 38),
	SCTLR_TAG_CHECK_FAULT_EL0_SYNC = (0b01ULL << 38),
	SCTLR_TAG_CHECK_FAULT_EL0_ASYNC = (0b10ULL << 38),
	SCTLR_TAG_CHECK_FAULT_EL0_SYNC_READ_ASYNC_WRITE = (0b11ULL << 38),

	SCTLR_TAG_CHECK_FAULT_EL1_IGNORE = (0b00ULL << 40),
	SCTLR_TAG_CHECK_FAULT_EL1_SYNC = (0b01ULL << 40),
	SCTLR_TAG_CHECK_FAULT_EL1_ASYNC = (0b10ULL << 40),
	SCTLR_TAG_CHECK_FAULT_EL1_SYNC_READ_ASYNC_WRITE = (0b11ULL << 40),

	/* TWEDEL is a 4-bit number, so this can't be encoded nicely. */
}SCTLRBitfields;

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
	constexpr explicit FORCE_INLINE PageTableEntry() noexcept : Raw(0) {}
	constexpr explicit FORCE_INLINE PageTableEntry(PageTableEntry &&Other) noexcept : Raw(Other.Raw){}
	constexpr explicit FORCE_INLINE PageTableEntry(const PageTableEntry &Other) noexcept : Raw(Other.Raw){}
	constexpr explicit FORCE_INLINE PageTableEntry(const PageTableEntry &Other, UINT64 Attr) noexcept : Raw(Other.Raw | Attr){}
	constexpr ~PageTableEntry(){};

	/**
	 * \~english @brief Determines if this page table entry is valid or not
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if valid, FALSE otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsMapped() const { return this->GetBits(0, 1) != 0; }

	/**
	 * \~english @brief Determines if this page table entry is a block or not.
	 * \~english @details A block entry is the final level of the page table: 
	 * this entry controls all the physical memory assigned to it directly. 
	 * Normally, this is a 4KB page table, but this could be a larger size, 
	 * such as 2MB or 1GB, depending on how the page tables were allocated.
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if a block, FALSE otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsBlock() const { return this->GetBits(1, 1) == 0; }

	/**
	 * \~english @brief Determines if this page table entry is a table or not.
	 * \~english @details A table entry is a non-final level of the page table: 
	 * this entry will have each of it's elements point to either more tables, or blocks.
	 * In essence, this flag is equivalent to a "walk further" flag. If set, then
	 * traversing the page tables will need to look through another table.
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if a table, FALSE otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsTable() const { return this->GetBits(1, 1) != 0; }

	/**
	 * \~english @brief Obtains the index of the MAIR to use for this entry
	 * \~english @details The Memory Attribute Indirect Register controls 
	 * information on the utilization of this memory area. This would handle, 
	 * for example, if this is a device MMIO area, cacheable, etc.
	 * This parameter only makes sense on block entries: for tables, this
	 * is presumed always 0.
	 * \~english @author Brian Schnepp
	 * \~english @return The entry in the MAIR register that this entry uses
	 */
	[[nodiscard]] constexpr FORCE_INLINE MAIREntry GetMAIREntry() const { return static_cast<MAIREntry>(this->GetMaskedBits(2, 3)); };
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsNonSecure() const { return this->GetBits(5, 1) != 0; }

	/**
	 * \~english @brief Checks if this entry is accessible to userspace
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if user-accessible, false otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsUserAccessible() const { return this->GetBits(6, 1) != 0; }

	/**
	 * \~english @brief Checks if this entry is read-only, or read-write to usermode
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if user-accessible, false otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsUserReadOnly() const { return this->GetBits(7, 1) != 0; }
	[[nodiscard]] constexpr FORCE_INLINE PageSharableType GetSharable() const { return static_cast<PageSharableType>(this->GetMaskedBits(8, 2)); };
	[[nodiscard]] constexpr FORCE_INLINE PageAccessed GetAccessor() const { return static_cast<PageAccessed>(this->GetMaskedBits(10, 1)); };

	/**
	 * \~english @brief Obtains the value of the Execute Never bit for the kernel at this level
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if not executable, false otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsKernelNoExecute() const { return this->GetBits(53, 1) != 0; };

	/**
	 * \~english @brief Obtains the value of the Execute Never bit for user code at this level
	 * \~english @author Brian Schnepp
	 * \~english @return TRUE if not executable, false otherwise.
	 */
	[[nodiscard]] constexpr FORCE_INLINE BOOL IsUserNoExecute() const { return this->GetBits(54, 1) != 0; };

	[[nodiscard]] constexpr FORCE_INLINE PhysicalAddress GetPhysicalAddressArea() const { return static_cast<PhysicalAddress>(this->GetMaskedBits(12, 48)); };
	[[nodiscard]] constexpr FORCE_INLINE UINT64 GetRawAttributes() const { return this->Raw; }
	[[nodiscard]] constexpr FORCE_INLINE PagePermissionRaw GetReadPermission() { return static_cast<PagePermissionRaw>(this->GetMaskedBits(6, 2)); }
	[[nodiscard]] constexpr FORCE_INLINE PagePermissionRaw GetWritePermission() { return static_cast<PagePermissionRaw>(this->GetMaskedBits(6, 2)); }

	constexpr FORCE_INLINE VOID SetRawAttributes(UINT64 Value) { this->Raw = Value; };
	constexpr FORCE_INLINE PageTableEntry &operator=(const PageTableEntry &Other) 
	{ 
		if (&Other != this) 
		{ 
			this->Raw = Other.Raw;
		}
		return *this;
	}

	/**
	 * \~english @brief Marks this page table entry as being valid or not
	 * \~english @param Value The value to set: TRUE for mapped, FALSE for not.
	 * \~english @author Brian Schnepp
	 */
	constexpr FORCE_INLINE VOID SetMapped(BOOL Value) { this->SetBits(0, 1, Value == 1); }

	/**
	 * \~english @brief Marks this page table entry as being a block or not
	 * \~english @param Value The value to set: TRUE for block, FALSE for not.
	 * \~english @author Brian Schnepp
	 * @see IsBlock
	 * @see IsTable
	 */
	constexpr FORCE_INLINE VOID SetBlock(BOOL Value) { this->SetBits(1, 1, Value == 0); }

	/**
	 * \~english @brief Marks this page table entry as being a table or not
	 * \~english @param Value The value to set: TRUE for table, FALSE for not.
	 * \~english @author Brian Schnepp
	 * @see IsBlock
	 * @see IsTable
	 */
	constexpr FORCE_INLINE VOID SetTable(BOOL Value) { this->SetBits(1, 1, Value == 1); }

	/**
	 * \~english @brief Sets the MAIR index for this block entry. Must be a block entry.
	 * \~english @param Value The index into the MAIR to use
	 * \~english @author Brian Schnepp
	 * @see GetMAIREntry
	 */
	constexpr FORCE_INLINE VOID SetMAIREntry(MAIREntry Value) { this->SetBitsRaw(2, 3, Value); }
	constexpr FORCE_INLINE VOID SetNonSecure(BOOL Value) { this->SetBits(5, 1, Value != 0); }

	/**
	 * \~english @brief Sets this block as being accessible to userspace or not
	 * \~english @param Value TRUE for being accessible to userspace, false otherwise
	 * \~english @author Brian Schnepp
	 */
	constexpr FORCE_INLINE VOID SetUserAccessible(BOOL Value) { this->SetBits(6, 1, Value != 0); }

	/**
	 * \~english @brief Sets this block as being read-only or read-write in userspace
	 * \~english @param Value TRUE for being read-only in userspace, false otherwise
	 * \~english @author Brian Schnepp
	 */
	constexpr FORCE_INLINE VOID SetUserReadOnly(BOOL Value) { this->SetBits(7, 1, Value != 0); }
	constexpr FORCE_INLINE VOID SetSharable(PageSharableType Value) { this->SetBitsRaw(8, 2, Value); }
	constexpr FORCE_INLINE VOID SetAccessor(PageAccessed Value) { this->SetBitsRaw(10, 1, Value); };

	/**
	 * \~english @brief Sets this block as not being executable in kernel space
	 * \~english @param Value TRUE for not-executable in kernel space, FALSE otherwise
	 * \~english @author Brian Schnepp
	 */
	constexpr FORCE_INLINE VOID SetKernelNoExecute(BOOL Value) { this->SetBits(53, 1, Value != 0); };

	/**
	 * \~english @brief Sets this block as not being executable in userspace
	 * \~english @param Value TRUE for not-executable in userspace, FALSE otherwise
	 * \~english @author Brian Schnepp
	 */
	constexpr FORCE_INLINE VOID SetUserNoExecute(BOOL Value) { this->SetBits(54, 1, Value != 0); };
	
	constexpr FORCE_INLINE VOID SetPagePermissions(UINT64 Val) 
	{ 
		this->SetKernelNoExecute(FALSE);
		this->SetUserNoExecute(FALSE);
		this->SetBitsRaw(6, 2, 0);
		this->Raw |= Val;
	}
	
	constexpr FORCE_INLINE VOID SetPhysicalAddressArea(PhysicalAddress Addr) { this->SetBitsRaw(12, 48-12, 0); this->Raw |= Addr & 0xFFFFFFFFFFFF000; }

protected:
	[[nodiscard]] 
	constexpr FORCE_INLINE UINT64 GetBits(UINT64 Offset, UINT64 Count) const
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + static_cast<UINT64>((Offset > 64) * (64));
		Count = ((Count <= 64) * (Count)) + static_cast<UINT64>((Count > 64) * (64));
		UINT64 Mask = (1ULL << Count) - 1;
		UINT64 Shifted = (this->Raw >> Offset);
		return Shifted & Mask;
	}

	[[nodiscard]] 
	constexpr FORCE_INLINE UINT64 GetMaskedBits(UINT64 Offset, UINT64 Count) const
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + static_cast<UINT64>((Offset > 64) * (64));
		Count = ((Count <= 64) * (Count)) + static_cast<UINT64>((Count > 64) * (64));
		UINT64 Mask = (1ULL << Count) - 1;
		return (this->Raw) & ((Mask << Offset) & Mask);
	}

	constexpr FORCE_INLINE VOID SetBits(UINT64 Offset, UINT64 Count, UINT64 Value)
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + static_cast<UINT64>((Offset > 64) * (64));
		Count = ((Count <= 64) * (Count)) + static_cast<UINT64>((Count > 64) * (64));
		const UINT64 Mask = ((1ULL << Count) - 1);
		this->Raw &= ~(Mask << Offset);
		Value &= Mask;
		this->Raw |= Value << Offset;
	}

	constexpr FORCE_INLINE VOID SetBitsRaw(UINT64 Offset, UINT64 Count, UINT64 Value)
	{
		/* Enforce offset and count can not be any more than 64 bits. */
		Offset = ((Offset <= 64) * (Offset)) + static_cast<UINT64>((Offset > 64) * (64));
		Count = ((Count <= 64) * (Count)) + static_cast<UINT64>((Count > 64) * (64));
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

FORCE_INLINE VirtualAddress PhysicalToVirtualAddress(PhysicalAddress PhyAddr)
{
	return PhyAddr | (0b1111111111111111ULL << 48);
}

FORCE_INLINE PhysicalAddress VirtualToPhysicalAddress(PageTable *RootTable, VirtualAddress Addr)
{
	/* TODO: traverse page table */
	PANTHEON_UNUSED(RootTable);
	return Addr;
}

VOID InvalidateTLB();

static_assert(sizeof(PageTableEntry) == sizeof(PageTableEntryRaw));

/* Note that everything in PageAllocator has to be marked as FORCE_INLINE,
 * since this code can also be called in the pre-kernel area 
 * (and thus page tables are not set up yet)
 */

class PageAllocator
{
public:
	FORCE_INLINE PageAllocator() : PageAllocator(nullptr, 0) {}

	FORCE_INLINE PageAllocator(VOID *Area, UINT64 Pages)
	{
		this->Allocator = pantheon::mm::SlabCache<pantheon::vmm::PageTable>(Area, Pages);
	}
	
	FORCE_INLINE ~PageAllocator()
	{

	}

	FORCE_INLINE pantheon::vmm::PageTable *Allocate()
	{
		pantheon::vmm::PageTable *NewTable = this->Allocator.AllocateNoCtor();
		ClearBuffer((char*)NewTable, sizeof(pantheon::vmm::PageTable));
		return NewTable;
	}

	[[nodiscard]] FORCE_INLINE UINT64 SpaceLeft() const
	{
		return this->Allocator.SpaceLeft();
	}

	[[nodiscard]] FORCE_INLINE UINT64 SpaceUsed() const
	{
		return this->Allocator.SlabCount() - this->SpaceLeft();
	}

	FORCE_INLINE BOOL Map(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, pantheon::vmm::PhysicalAddress PhysAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		if (Size == 0)
		{
			return TRUE;
		}

		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);
		PhysAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* Start mapping everything */
		while (Size > 0)
		{
			/* Can we find the L0 table? */
			UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);

			if (L0Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L0, and write it in. */
				CreateTable(L0Entry, TTBR);
			}

			/* We can't have L0 blocks, so don't try to fit it in: this would
			* only be possible for 5-level paging, and 512GB pages are silly
			* right now anyway.
			*/

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)(L0Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);


			namespace BlockSize = pantheon::vmm::BlockSize;
			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L1Entry, Permissions, BlockSize::L1BlockSize))
			{
				continue;
			}

			if (L1Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L1, and write it in. */
				CreateTable(L1Entry, TTBR);		
			}

			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			/* We have a (greedy) opportunity to save page tables. Try making this a block if we can. */
			if (CreateGreedyBlock(Size, VirtAddr, PhysAddr, L2Entry, Permissions, BlockSize::L2BlockSize))
			{
				continue;
			}

			if (L2Entry->IsMapped() == FALSE)
			{
				/* Allocate a new L2, and write it in. */
				CreateTable(L2Entry, TTBR);
			}

			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			/* We can't make a "bigger" chunk from L3: L3 is already the smallest size. */

			/* Allocate a new L3, and write it in. */
			CreateBlock(L3Entry, Permissions, PhysAddr);
			L3Entry->SetTable(TRUE);	/* We need this to be 1??? */
			Size -= BlockSize::L3BlockSize;
			PhysAddr += BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;

		}
		VMMSync();
		return TRUE;
	}

	FORCE_INLINE BOOL Reprotect(pantheon::vmm::PageTable *TTBR, pantheon::vmm::VirtualAddress VirtAddr, UINT64 Size, const pantheon::vmm::PageTableEntry &Permissions)
	{
		if (Size == 0)
		{
			return TRUE;
		}
			
		/* Assert that Size is a multiple of the page size. If not, round anyway. */
		Size = Align<UINT64>(Size, pantheon::vmm::BlockSize::L3BlockSize);

		/* Make sure VirtAddr and PhysAddr are aligned to the page size too. */
		VirtAddr &= ~(pantheon::vmm::BlockSize::L3BlockSize - 1);

		/* We don't have panic yet, so for now just return false. */
		pantheon::Sync::DSBISH();
		if (TTBR == nullptr)
		{
			VMMSync();
			return FALSE;
		}

		/* What indices might we have to look up? */
		UINT16 L0Index = VirtAddrToPageTableIndex(VirtAddr, 0);
		namespace BlockSize = pantheon::vmm::BlockSize;

		/* Start mapping everything */
		while (Size > 0)
		{
			pantheon::vmm::PageTableEntry *L0Entry = &(TTBR->Entries[L0Index]);
			if (L0Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L1Index = VirtAddrToPageTableIndex(VirtAddr, 1);
			pantheon::vmm::PageTable *L1 = (pantheon::vmm::PageTable*)(L0Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L1Entry = &(L1->Entries[L1Index]);

			if (L1Entry == nullptr || L1Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L1Entry->IsBlock())
			{
				/* We'll need to split this block up. */

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L1BlockSize - 1);
				UINT64 Diff = VirtAddr & (BlockSize::L1BlockSize - 1);

				/* These names are to reflect the above. */
				pantheon::vmm::PhysicalAddress PhysMasked = L1Entry->GetPhysicalAddressArea();
				pantheon::vmm::PhysicalAddress PhysAddr = PhysMasked + Diff; 

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L1Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				L1Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->Map(TTBR, VirtMasked, PhysMasked, Diff, OldEntry);
				Status &= this->Map(TTBR, VirtAddr, PhysAddr, Size, Permissions);
				Status &= this->Map(TTBR, VirtAddr + Size, PhysAddr + Size, BlockSize::L1BlockSize - Size - Diff, OldEntry);

				VMMSync();
				return Status;
			}

			/* Otherwise, we need to walk down the table further. */


			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L2Index = VirtAddrToPageTableIndex(VirtAddr, 2);
			pantheon::vmm::PageTable *L2 = (pantheon::vmm::PageTable*)(L1Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L2Entry = &(L2->Entries[L2Index]);

			if (L2Entry == nullptr || L2Entry->IsMapped() == FALSE)
			{
				/* We don't have something to reprotect: mem doesnt exist. */
				goto fail;
			}

			/* Was this a section? */
			if (L2Entry->IsBlock())
			{
				/* We'll need to split this block up. TODO. */
				pantheon::vmm::PhysicalAddress PhysAddr = L2Entry->GetPhysicalAddressArea();

				UINT64 VirtMasked = VirtAddr & ~(BlockSize::L2BlockSize - 1);
				UINT64 Diff = VirtAddr - VirtMasked;

				/* We now have some different sized area to worry about:
				* 1. A mapping using the old permissions, from PhysAddr, to PhysAddr + Diff
				* 2. A mapping using the new permissions, from PhysAddr + Diff to PhysAddr + Diff + min(Size, L1BlockSize)
				* 3. A mapping using the old permissions, from PhysAddr + Diff + min(Size, L1BlockSize) to the end of the L1 block size.
				*/

				/* First, copy the old attributes. */
				pantheon::vmm::PageTableEntry OldEntry;
				OldEntry.SetRawAttributes(L2Entry->GetRawAttributes());
				OldEntry.SetPhysicalAddressArea(0x00);

				/* Next, smash the L2 entry. */
				L2Entry->SetRawAttributes(0x00);

				/* Now remap these areas... */
				BOOL Status = TRUE;
				Status &= this->Map(TTBR, VirtMasked, PhysAddr, Diff, OldEntry);
				Status &= this->Map(TTBR, VirtAddr, PhysAddr + Diff, Size, Permissions);
				Status &= this->Map(TTBR, VirtAddr + Size, PhysAddr + Diff + Size, BlockSize::L2BlockSize - Size - Diff, OldEntry);

				return Status;
			}

			/* Otherwise, we need to walk down the table further. */

			/* TODO: We need to handle virtual addresses!!! These pointers are physical memory pointers. */
			UINT16 L3Index = VirtAddrToPageTableIndex(VirtAddr, 3);
			pantheon::vmm::PageTable *L3 = (pantheon::vmm::PageTable*)(L2Entry->GetPhysicalAddressArea());
			pantheon::vmm::PageTableEntry *L3Entry = &(L3->Entries[L3Index]);

			if (L3)
			{
				if (L3Entry->IsMapped() == FALSE)
				{
					/* We don't have something to reprotect: mem doesnt exist. */
					goto fail;
				}

				/* There is no lower level: we'll reprotect this, since it exists. */
				pantheon::vmm::PhysicalAddress PhysAddr = L3Entry->GetPhysicalAddressArea();
				CreateBlock(L3Entry, Permissions, PhysAddr);
				L3Entry->SetTable(TRUE);	/* This is sort of weird: trust that this is right. */
			}
			else
			{
				goto fail;
			}
			
			Size -= BlockSize::L3BlockSize;
			VirtAddr += BlockSize::L3BlockSize;
		}
		VMMSync();
		return TRUE;

	fail:
		VMMSync();
		return FALSE;	
	}	

private:
	static FORCE_INLINE void VMMSync()
	{
		pantheon::Sync::DSBISH();
		pantheon::Sync::ISB();		
	}

	static FORCE_INLINE UINT16 VirtAddrToPageTableIndex(pantheon::vmm::VirtualAddress VAddr, UINT8 Level)
	{
		/* Only valid for 0, 1, 2, or 3. */
		Level = (Level > 3) ? 3 : Level;
		UINT64 Shift = 39 - (Level * 9);
		return (VAddr >> Shift) & 0x1FF;
	}

	FORCE_INLINE void CreateBlock(pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, pantheon::vmm::PhysicalAddress Address)
	{
		Entry->SetRawAttributes(Permissions.GetRawAttributes());
		Entry->SetPhysicalAddressArea(Address);
		Entry->SetBlock(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE void CreateTable(pantheon::vmm::PageTableEntry *Entry, pantheon::vmm::PageTable *TTBR)
	{
		pantheon::vmm::PageTable *Table = this->Allocate();
		Entry->SetPhysicalAddressArea(VirtualToPhysicalAddress(TTBR, (UINT64)Table));
		Entry->SetTable(TRUE);
		Entry->SetMapped(TRUE);
		pantheon::Sync::DSBISH();
	}

	FORCE_INLINE BOOL CreateGreedyBlock(UINT64 &Size, pantheon::vmm::VirtualAddress &VirtAddr, pantheon::vmm::PhysicalAddress &PhysAddr, pantheon::vmm::PageTableEntry *Entry, const pantheon::vmm::PageTableEntry &Permissions, UINT64 BlockSize)
	{
		if (Size >= BlockSize
			&& IsAligned<UINT64>(VirtAddr, BlockSize)
			&& IsAligned<UINT64>(PhysAddr, BlockSize))
		{
			CreateBlock(Entry, Permissions, PhysAddr);
			Size -= BlockSize;
			PhysAddr += BlockSize;
			VirtAddr += BlockSize;
			return TRUE;
		}
		return FALSE;
	}

	pantheon::mm::SlabCache<pantheon::vmm::PageTable> Allocator;
};

}

#endif