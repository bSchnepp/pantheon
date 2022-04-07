#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_slab.hpp>

#include <System/PhyMemory/kern_alloc.hpp>

/* This needs to be replaced with a proper slab allocator at some point */
static constexpr UINT64 InitNumPageTables = 1024;
alignas(4096) static pantheon::vmm::PageTable Tables[InitNumPageTables];
static pantheon::vmm::PageAllocator PageTableAllocator;

/**
 * @file System/Proc/kern_proc.cpp
 * @brief Defines a Pantheon process, which contains a page table and some threads of execution
 */

/**
 * @brief Initializes the necessary mechanisms to setup thread and process allocation
 */
void pantheon::InitProcessTables()
{
	Thread::Init();
	Process::Init();
	PageTableAllocator = pantheon::vmm::PageAllocator(Tables, InitNumPageTables);
}

/**
 * @brief Creates a minimal, empty process, which by default is equivalent to the idle process
 * @author Brian Schnepp
 */
pantheon::Process::Process() : pantheon::Lockable("Process")
{
	this->CurState = pantheon::Process::STATE_INIT;
	this->CurPriority = pantheon::Process::PRIORITY_VERYLOW;
	this->ProcessString = "idle";
	this->PID = 0;
}

pantheon::Process::~Process() = default;


/**
 * @brief Initializes a process with appropriate state. Must be locked before use.
 * @details A process is initialized with state as needed from CreateInfo, where it is used to set up initial memory mapped locations, a user mode stack, a name, and the appropriate entry point for it.
 * @param CreateInfo The process creation info
 */
void pantheon::Process::Initialize(const pantheon::ProcessCreateInfo &CreateInfo)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		pantheon::StopError("Process not locked in initialization");
	}
	this->CurState = pantheon::Process::STATE_INIT;
	this->CurPriority = pantheon::Process::PRIORITY_NORMAL;
	this->ProcessString = CreateInfo.Name;
	this->PID = pantheon::AcquireProcessID();
	this->TTBR0 = pantheon::PageAllocator::Alloc();
	this->EntryPoint = CreateInfo.EntryPoint;

	pantheon::vmm::VirtualAddress NewTableVAddr = pantheon::vmm::PhysicalToVirtualAddress(this->TTBR0);
	pantheon::vmm::PageTable *PgTable = reinterpret_cast<pantheon::vmm::PageTable*>(NewTableVAddr);

	this->MemoryMap = PgTable;
	this->MemoryMap->Clear();

	/* Let's go ahead and map in everything in the lower table as-is. */
	pantheon::vmm::PageTableEntry Entry;
	Entry.SetBlock(TRUE);
	Entry.SetMapped(TRUE);
	Entry.SetUserNoExecute(TRUE);
	Entry.SetKernelNoExecute(FALSE);

	/* For now, allow anything since we don't have a real userland yet.
	 * We'll need to create page allocators for userland at some point,
	 * but for now let everything share the same address space.
	 */
	UINT64 PagePermission = 0;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_KERN;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_KERN; 
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_READ_WRITE_USER;
	PagePermission |= pantheon::vmm::PAGE_PERMISSION_EXECUTE_USER;
	Entry.SetPagePermissions(PagePermission);

	Entry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
	Entry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	Entry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

	pantheon::vmm::VirtualAddress VAddrs[Process::StackPages];
	pantheon::vmm::PhysicalAddress PAddrs[Process::StackPages];

	for (UINT8 Index = 0; Index < Process::StackPages; Index++)
	{
		PAddrs[Index] = pantheon::PageAllocator::Alloc();
		VAddrs[Index] = StackAddr - pantheon::vmm::SmallestPageSize * Index;
	}

	pantheon::vmm::PageTableEntry UStackEntry = pantheon::vmm::StackPermissions();

	/* Create the stack */
	for (UINT64 Index = 0; Index < Process::StackPages; ++Index)
	{
		this->MapAddress(VAddrs[Index], PAddrs[Index], UStackEntry);
	}

	/* Map in initial objects */
	for (UINT64 Index = 0; Index < CreateInfo.NumMemoryRegions; ++Index)
	{
		this->MapAddress(CreateInfo.VPages[Index], CreateInfo.PPages[Index], CreateInfo.Permissions[Index]);
	}

	this->EntryPoint = CreateInfo.EntryPoint;
	
	pantheon::Sync::DSBISH();
	pantheon::Sync::ISB();
}

pantheon::Process &pantheon::Process::operator=(const pantheon::Process &Other)
{
	OBJECT_SELF_ASSERT();
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->CurPriority = Other.CurPriority;
	this->ProcessString = Other.ProcessString;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;
	return *this;
}

pantheon::Process &pantheon::Process::operator=(pantheon::Process &&Other) noexcept
{
	OBJECT_SELF_ASSERT();
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->CurPriority = Other.CurPriority;
	this->ProcessString = Other.ProcessString;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;	
	ClearBuffer((CHAR*)&Other, sizeof(Process));
	return *this;
}

/**
 * @brief Maps a given physical address to a given virtual address. The smallest page size supported is assumed for granularity. Process must be locked before use.
 * @param VAddresses The virtual address in this process to map to
 * @param PAddresses The physical address on this machine to map from
 * @param PageAttributes The configuration to be used for the given page
 */
void pantheon::Process::MapAddress(const pantheon::vmm::VirtualAddress &VAddress, const pantheon::vmm::PhysicalAddress &PAddress, const pantheon::vmm::PageTableEntry &PageAttributes)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked when mapping addresses\n");
	}

	PageTableAllocator.Map(this->MemoryMap, VAddress, PAddress, pantheon::vmm::SmallestPageSize, PageAttributes);
}

/**
 * @brief Obtains the process state for this process
 * @return The state of this process
 */
[[nodiscard]] 
pantheon::Process::State pantheon::Process::MyState() const
{
	OBJECT_SELF_ASSERT();
	/* ubsan says there is an error here */
	if (this->CurState > pantheon::Process::STATE_MAX)
	{
		StopErrorFmt("Invalid process state: got 0x%lx\n", this->CurState);
	}
	return this->CurState;
}

/**
 * @brief Sets the state of this process. Process must be locked before use.
 */
void pantheon::Process::SetState(pantheon::Process::State State)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with SetState");
	}

	this->CurState = State;
}

/**
 * @brief Attaches a handle to this process. Process must be locked before use.
 * @return An integer as an index into this current handle table, or a negative number on an error.
 */
INT32 pantheon::Process::EncodeHandle(const pantheon::Handle &NewHand)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with EncodeHandle");
	}
	return this->HandTable.Create(NewHand);
}

/**
 * @brief Obtains a pointer to a handle based on the index provided. Must be locked before use.
 * @return A pointer to a handle corresponding to the index provided. Nullptr if invalid.
 */
pantheon::Handle *pantheon::Process::GetHandle(INT32 HandleID)
{
	OBJECT_SELF_ASSERT();
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with GetHandle");
	}

	return this->HandTable.Get(HandleID);
}

/**
 * @brief Obtains the physical address of the page table for this process.
 * @return The physical address of the page table for this process.
 */
[[nodiscard]] 
pantheon::vmm::PhysicalAddress pantheon::Process::GetTTBR0() const
{
	OBJECT_SELF_ASSERT();
	return this->TTBR0;
}

/**
 * @brief Obtains the page table in kernel virtual memory for this process.
 * @return A pointer to the page table for this process
 */
[[nodiscard]] 
pantheon::vmm::PageTable *pantheon::Process::GetPageTable() const
{
	OBJECT_SELF_ASSERT();
	return this->MemoryMap;
}
	