#include <arch.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <Sync/kern_spinlock.hpp>

#include <Proc/kern_thread.hpp>
#include <Handle/kern_handle.hpp>
#include <Handle/kern_lockable.hpp>

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
	BOOL CreateThread(void *StartAddr, void *ThreadData);
	BOOL CreateThread(void *StartAddr, void *ThreadData, pantheon::ThreadPriority Priority);

	[[nodiscard]] ProcessState MyState() const;
	void SetState(ProcessState State);

	void MapPages(pantheon::vmm::VirtualAddress *VAddresses, pantheon::vmm::PhysicalAddress *PAddresses, pantheon::vmm::PageTableEntry *PageAttributes, UINT64 NumPages);

	INT64 EncodeHandle(const pantheon::Handle &NewHand);
	pantheon::Handle *GetHandle(UINT8 HandleID);
	[[nodiscard]] void *GetTTBR0() const;

private:
	UINT32 PID;
	String ProcessCommand;
	
	ProcessState CurState;
	ProcessPriority Priority;

	/* Note that TTBR0 refers to the physical address of MemoryMap. */
	void *TTBR0;
	pantheon::vmm::PageTable *MemoryMap;

	static constexpr UINT64 HandleTableSize = 64;
	pantheon::Handle ProcHandleTable[HandleTableSize];
};

}

#endif