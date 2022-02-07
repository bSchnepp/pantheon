#include <arch.hpp>

#include <vmm/pte.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <Sync/kern_spinlock.hpp>

#include <Proc/kern_thread.hpp>
#include <Handle/kern_handle.hpp>
#include <Handle/kern_lockable.hpp>
#include <Handle/kern_handletable.hpp>

#ifndef _KERN_PROC_HPP_
#define _KERN_PROC_HPP_

namespace pantheon
{

class Thread;

typedef enum ProcessState
{
	PROCESS_STATE_INIT,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_BLOCKED,
	PROCESS_STATE_ZOMBIE,
	PROCESS_STATE_TERMINATED,
	PROCESS_STATE_MAX,
}ProcessState;

typedef enum ProcessPriority
{
	PROCESS_PRIORITY_VERYLOW = 0,
	PROCESS_PRIORITY_LOW = 1,
	PROCESS_PRIORITY_NORMAL = 2,
	PROCESS_PRIORITY_HIGH = 3,
	PROCESS_PRIORITY_VERYHIGH = 4,
}ProcessPriority;

class Process : public pantheon::Lockable
{
public:
	Process();
	Process(const char *CommandString);
	Process(String &CommandString);
	Process(const Process &Other) noexcept;
	Process(Process &&Other) noexcept;
	~Process() override;

	Process &operator=(const Process &Other);
	Process &operator=(Process &&Other) noexcept;

	[[nodiscard]] const String &GetProcessString() const;
	[[nodiscard]] UINT32 ProcessID() const;

	[[nodiscard]] ProcessState MyState() const;
	void SetState(ProcessState State);

	void SetPageTable(pantheon::vmm::PageTable *Root, pantheon::vmm::PhysicalAddress PageTablePhysicalAddr);
	void MapAddresses(pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes, UINT64 NumPages);

	INT64 EncodeHandle(const pantheon::Handle &NewHand);
	pantheon::Handle *GetHandle(INT32 HandleID);
	[[nodiscard]] pantheon::vmm::PhysicalAddress GetTTBR0() const;

	static const constexpr pantheon::vmm::VirtualAddress StackAddr = 0x7FC0000000;

private:
	UINT32 PID;
	String ProcessCommand;
	
	ProcessState CurState;
	ProcessPriority Priority;

	/* Note that TTBR0 refers to the physical address of MemoryMap. */
	pantheon::vmm::PhysicalAddress TTBR0;
	pantheon::vmm::PageTable *MemoryMap;
	pantheon::HandleTable HandTable;

private:
	void CreateBlankPageTable();

};

void InitProcessTables();

}

#endif