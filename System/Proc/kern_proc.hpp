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

#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_PROC_HPP_
#define _KERN_PROC_HPP_

namespace pantheon
{

class Thread;

typedef enum ProcessCreateFlags
{
	PROCESS_CREATE_FLAG_MAX,
}ProcessCreateFlags;

typedef struct ProcessCreateInfo
{
	String Name;
	pantheon::vmm::VirtualAddress EntryPoint;

	UINT64 NumMemoryRegions;
	pantheon::vmm::VirtualAddress *VPages;
	pantheon::vmm::PhysicalAddress *PPages;
	pantheon::vmm::PageTableEntry *Permissions;

}ProcessCreateInfo;

class Process : public pantheon::Allocatable<Process, 128>, public pantheon::Lockable
{
public:
	typedef enum State
	{
		STATE_INIT,
		STATE_RUNNING,
		STATE_ZOMBIE,
		STATE_TERMINATED,
		STATE_MAX,
	}ProcessState;

	typedef enum Priority
	{
		PRIORITY_VERYLOW = 0,
		PRIORITY_LOW = 1,
		PRIORITY_NORMAL = 2,
		PRIORITY_HIGH = 3,
		PRIORITY_VERYHIGH = 4,
	}ProcessPriority;

	typedef UINT32 ID;

public:
	Process();
	~Process() override;

	void Initialize(const ProcessCreateInfo &CreateInfo);

	Process &operator=(const Process &Other);
	Process &operator=(Process &&Other) noexcept;

	[[nodiscard]] const String &GetProcessString() const;
	[[nodiscard]] UINT32 ProcessID() const;

	[[nodiscard]] ProcessState MyState() const;
	void SetState(ProcessState State);
	void MapAddress(const pantheon::vmm::VirtualAddress &VAddresses, const pantheon::vmm::PhysicalAddress &PAddresses, const pantheon::vmm::PageTableEntry &PageAttributes);

	INT64 EncodeHandle(const pantheon::Handle &NewHand);
	pantheon::Handle *GetHandle(INT32 HandleID);

	[[nodiscard]] pantheon::vmm::PhysicalAddress GetTTBR0() const;
	[[nodiscard]] pantheon::vmm::PageTable *GetPageTable() const;

	static const constexpr UINT64 StackPages = 16;
	static const constexpr pantheon::vmm::VirtualAddress StackAddr = 0xFFFFFFFFF000;

	[[nodiscard]] pantheon::vmm::VirtualAddress GetEntryPoint() const { return this->EntryPoint; }

private:
	ID PID;
	String ProcessString;
	
	State CurState;
	Priority CurPriority;

	pantheon::vmm::VirtualAddress EntryPoint;

	/* Note that TTBR0 refers to the physical address of MemoryMap. */
	pantheon::vmm::PhysicalAddress TTBR0;
	pantheon::vmm::PageTable *MemoryMap;

	pantheon::HandleTable HandTable;
};

void InitProcessTables();

}

#endif