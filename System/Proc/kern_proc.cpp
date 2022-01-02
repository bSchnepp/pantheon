#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

#include <System/Proc/kern_proc.hpp>
#include <System/Proc/kern_sched.hpp>
#include <System/Proc/kern_thread.hpp>

#include <Handle/kern_lockable.hpp>
#include <Common/Structures/kern_slab.hpp>

/* This needs to be replaced with a proper slab allocator at some point */
static constexpr UINT64 InitNumPageTables = 1024;
static pantheon::vmm::PageTable Tables[InitNumPageTables];
static pantheon::vmm::PageAllocator PageTableAllocator;

void pantheon::InitProcessTables()
{
	PageTableAllocator = pantheon::vmm::PageAllocator(Tables, InitNumPageTables);
}

pantheon::Process::Process() : pantheon::Lockable("Process")
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_VERYLOW;
	this->ProcessCommand = "idle";
	this->PID = 0;
	/* TODO: Create new page tables, instead of reusing old stuff. */
	this->TTBR0 = pantheon::CPUReg::R_TTBR0_EL1();
	this->MemoryMap = nullptr;
	
}

pantheon::Process::Process(const char *CommandString) : pantheon::Lockable("Process")
{
	this->ProcessCommand = pantheon::String(CommandString);
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->PID = pantheon::AcquireProcessID();
	/* TODO: Create new page tables, instead of reusing old stuff. */
	this->TTBR0 = pantheon::CPUReg::R_TTBR0_EL1();
	this->MemoryMap = nullptr;
	
}

pantheon::Process::Process(pantheon::String &CommandString) : pantheon::Lockable("Process")
{
	this->CurState = pantheon::PROCESS_STATE_INIT;
	this->Priority = pantheon::PROCESS_PRIORITY_NORMAL;
	this->ProcessCommand = CommandString;
	this->PID = pantheon::AcquireProcessID();
	/* TODO: Create new page tables, instead of reusing old stuff. */
	this->TTBR0 = pantheon::CPUReg::R_TTBR0_EL1();
	this->MemoryMap = nullptr;
	
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

pantheon::Process &pantheon::Process::operator=(const pantheon::Process &Other)
{
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
	return this->ProcessCommand;
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData)
{
	return this->CreateThread(StartAddr, ThreadData, pantheon::THREAD_PRIORITY_NORMAL);
}

BOOL pantheon::Process::CreateThread(void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with CreateThread");
	}

	BOOL Status = pantheon::GetGlobalScheduler()->CreateThread(this, StartAddr, ThreadData, Priority);
	return Status;
}

void pantheon::Process::CreateBlankPageTable()
{
	/* HACK: we need to properly devirtualize addresses! */
	this->MemoryMap = PageTableAllocator.Allocate();
	this->TTBR0 = (UINT64)this->MemoryMap;
}

void pantheon::Process::SetPageTable(pantheon::vmm::PageTable *Root, pantheon::vmm::PhysicalAddress PageTablePhysicalAddr)
{
	this->MemoryMap = Root;
	this->TTBR0 = PageTablePhysicalAddr;
}

void pantheon::Process::MapPages(pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, pantheon::vmm::PageTableEntry *PageAttributes, UINT64 NumPages)
{
	for (UINT64 Num = 0; Num < NumPages; Num++)
	{
		PageTableAllocator.Map(this->MemoryMap, VAddresses[Num], PAddresses[Num], pantheon::vmm::SmallestPageSize, PageAttributes[Num]);
	}
}

[[nodiscard]]
UINT32 pantheon::Process::ProcessID() const
{
	return this->PID;
}

[[nodiscard]] 
pantheon::ProcessState pantheon::Process::MyState() const
{
	return this->CurState;
}

void pantheon::Process::SetState(pantheon::ProcessState State)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with SetState");
	}

	this->CurState = State;
}

INT64 pantheon::Process::EncodeHandle(const pantheon::Handle &NewHand)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with EncodeHandle");
	}

	for (UINT64 Index = 0; Index < pantheon::Process::HandleTableSize; Index++)
	{
		if (this->ProcHandleTable[Index].IsValid() == FALSE)
		{
			this->ProcHandleTable[Index] = NewHand;
			return static_cast<INT64>(Index);
		}
	}
	return -1;
}

pantheon::Handle *pantheon::Process::GetHandle(UINT8 HandleID)
{
	if (this->IsLocked() == FALSE)
	{
		StopError("Process not locked with GetHandle");
	}

	if (HandleID > pantheon::Process::HandleTableSize)
	{
		return nullptr;
	}

	pantheon::Handle *CurHandle = &(this->ProcHandleTable[HandleID]);
	if (CurHandle->IsValid())
	{
		return CurHandle;
	}
	return nullptr;
}

[[nodiscard]] 
pantheon::vmm::PhysicalAddress pantheon::Process::GetTTBR0() const
{
	return this->TTBR0;
}