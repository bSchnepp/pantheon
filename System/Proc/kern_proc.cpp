#include <vmm/pte.hpp>
#include <vmm/vmm.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <Handle/kern_lockable.hpp>
#include <Common/Structures/kern_slab.hpp>

#include <System/PhyMemory/kern_alloc.hpp>

/* This needs to be replaced with a proper slab allocator at some point */
static constexpr UINT64 InitNumPageTables = 1024;
alignas(4096) static pantheon::vmm::PageTable Tables[InitNumPageTables];
static pantheon::vmm::PageAllocator PageTableAllocator;

void pantheon::InitProcessTables()
{
	Thread::Init();
	Process::Init();
	PageTableAllocator = pantheon::vmm::PageAllocator(Tables, InitNumPageTables);
}

pantheon::Process::Process() : pantheon::Lockable("Process")
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_VERYLOW;
	this->ProcessCommand = "idle";
	this->PID = 0;
}

pantheon::Process::Process(const char *CommandString) : pantheon::Lockable("Process")
{
	this->ProcessCommand = pantheon::String(CommandString);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->PID = pantheon::AcquireProcessID();
	this->CreateBlankPageTable();
}

pantheon::Process::Process(pantheon::String &CommandString) : pantheon::Lockable("Process")
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
	this->CreateBlankPageTable();
}

pantheon::Process::Process(const Process &Other) noexcept : pantheon::Lockable("Process")
{
	this->Lock();
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;
	this->Unlock();
}

pantheon::Process::Process(Process &&Other) noexcept
{
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;
	ClearBuffer((CHAR*)&Other, sizeof(Process));	
}

pantheon::Process::~Process()
{

}

void pantheon::Process::Initialize(const pantheon::ProcessCreateInfo &CreateInfo)
{
	OBJECT_SELF_ASSERT(this);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CreateInfo.Name;
	this->PID = pantheon::AcquireProcessID();
	this->CreateBlankPageTable();

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


	/* For the initial stack, it will be at 0x7FC0000000, 
	 * which should be ASLRed later. This is a very high virtual address!
	 * The initial user stack will be 4 pages.
	 */
	static constexpr UINT8 NumInitStackPages = 4;
	pantheon::vmm::VirtualAddress VAddrs[NumInitStackPages];
	pantheon::vmm::PhysicalAddress PAddrs[NumInitStackPages];

	for (UINT8 Index = 0; Index < NumInitStackPages; Index++)
	{
		PAddrs[Index] = pantheon::PageAllocator::Alloc();
		ClearBuffer((CHAR*)pantheon::vmm::PhysicalToVirtualAddress(PAddrs[Index]), pantheon::vmm::SmallestPageSize);
		VAddrs[Index] = StackAddr - pantheon::vmm::SmallestPageSize * Index;
	}

	pantheon::vmm::PageTableEntry UStackEntry = pantheon::vmm::StackPermissions();

	/* Create the stack */
	for (UINT64 Index = 0; Index < NumInitStackPages; ++Index)
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
	OBJECT_SELF_ASSERT(this);
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;
	return *this;
}

pantheon::Process &pantheon::Process::operator=(pantheon::Process &&Other) noexcept
{
	OBJECT_SELF_ASSERT(this);
	if (this == &Other)
	{
		return *this;
	}
	Lockable::operator=(Other);
	this->CurState = Other.CurState;
	this->PID = Other.PID;
	this->Priority = Other.Priority;
	this->ProcessCommand = Other.ProcessCommand;
	this->MemoryMap = Other.MemoryMap;
	this->TTBR0 = Other.TTBR0;	
	ClearBuffer((CHAR*)&Other, sizeof(Process));
	return *this;
}

[[nodiscard]]
const pantheon::String &pantheon::Process::GetProcessString() const
{
	OBJECT_SELF_ASSERT(this);
	return this->ProcessCommand;
}

extern "C" CHAR *USER_BEGIN;
extern "C" CHAR *USER_END;

void pantheon::Process::CreateBlankPageTable()
{
	OBJECT_SELF_ASSERT(this);
	this->TTBR0 = pantheon::PageAllocator::Alloc();

	pantheon::vmm::VirtualAddress NewTableVAddr = pantheon::vmm::PhysicalToVirtualAddress(this->TTBR0);
	pantheon::vmm::PageTable *PgTable = reinterpret_cast<pantheon::vmm::PageTable*>(NewTableVAddr);

	this->MemoryMap = PgTable;
	this->MemoryMap->Clear();

}

void pantheon::Process::SetPageTable(pantheon::vmm::PageTable *Root, pantheon::vmm::PhysicalAddress PageTablePhysicalAddr)
{
	OBJECT_SELF_ASSERT(this);
	this->MemoryMap = Root;
	this->TTBR0 = PageTablePhysicalAddr;
}

void pantheon::Process::MapAddress(const pantheon::vmm::VirtualAddress &VAddresses, const pantheon::vmm::PhysicalAddress &PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes)
{
	OBJECT_SELF_ASSERT(this);
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked when mapping addresses\n");
	}

	PageTableAllocator.Map(this->MemoryMap, VAddresses, PAddresses, pantheon::vmm::SmallestPageSize, PageAttributes);
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	OBJECT_SELF_ASSERT(this);
	return this->PID;
}

[[nodiscard]] 
pantheon::ProcessState pantheon::Process::MyState() const
{
	OBJECT_SELF_ASSERT(this);
	/* ubsan says there is an error here */
	if (this->CurState > pantheon::PROCESS_STATE_MAX)
	{
		StopErrorFmt("Invalid process state: got 0x%lx\n", this->CurState);
	}
	return this->CurState;
}

void pantheon::Process::SetState(pantheon::ProcessState State)
{
	OBJECT_SELF_ASSERT(this);
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with SetState");
	}

	this->CurState = State;
}

INT64 pantheon::Process::EncodeHandle(const pantheon::Handle &NewHand)
{
	OBJECT_SELF_ASSERT(this);
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with EncodeHandle");
	}
	return this->HandTable.Create(NewHand);
}

pantheon::Handle *pantheon::Process::GetHandle(INT32 HandleID)
{
	OBJECT_SELF_ASSERT(this);
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with GetHandle");
	}

	return this->HandTable.Get(HandleID);
}

[[nodiscard]] 
pantheon::vmm::PhysicalAddress pantheon::Process::GetTTBR0() const
{
	OBJECT_SELF_ASSERT(this);
	return this->TTBR0;
}

[[nodiscard]] 
pantheon::vmm::PageTable *pantheon::Process::GetPageTable() const
{
	OBJECT_SELF_ASSERT(this);
	return this->MemoryMap;
}
	