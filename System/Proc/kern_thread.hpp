#include <arch.hpp>
#include <vmm/vmm.hpp>

#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <kern_container.hpp>

#include <Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <kern_object.hpp>

#ifndef _KERN_THREAD_HPP_
#define _KERN_THREAD_HPP_

namespace pantheon
{

class Process;

class Thread  : public pantheon::Object<Thread, 512>, public pantheon::Lockable
{
public:
	typedef struct ThreadLocalHeader
	{
		/* [10 bits Size] [2 Bits ReqType] [4 Bits ReqTypeData] */
		UINT16 Meta;
		UINT16 Cmd;
		UINT32 ClientPID;

		[[nodiscard]] constexpr UINT16 GetSize() const
		{
			return (this->Meta >> 6) & 0x3FF;
		}

		[[nodiscard]] constexpr UINT16 GetReqType() const
		{
			return (this->Meta >> 4) & 0x3;
		}

		[[nodiscard]] constexpr UINT16 GetReqTypeData() const
		{
			return (this->Meta >> 0) & 0xF;
		}
	}ThreadLocalHeader;

	typedef union ThreadLocalPayload
	{
		UINT32 Data[1022];
	}ThreadLocalPayload;

	typedef struct ThreadLocalRegion
	{
		union
		{
			struct
			{
				ThreadLocalHeader Header;
				ThreadLocalPayload Payload;
			};
			UINT32 RawData[1024];
		};
	}ThreadLocalRegion;

	static_assert(sizeof(ThreadLocalRegion) == 4096);

public:
	typedef enum State
	{
		STATE_DEAD,
		STATE_INIT,
		STATE_RUNNING,
		STATE_WAITING,
		STATE_BLOCKED,
		STATE_TERMINATED,
		STATE_MAX,
	}State;

	typedef enum Priority
	{
		PRIORITY_VERYLOW = 0,
		PRIORITY_LOW = 1,
		PRIORITY_NORMAL = 2,
		PRIORITY_HIGH = 3,
		PRIORITY_VERYHIGH = 4,
		PRIORITY_MAX,
	}Priority;

public:
	Thread();
	Thread(Process *ParentProcess);
	Thread(Process *ParentProcess, Thread::Priority Priority);
	Thread(const Thread &Other);
	Thread(Thread &&Other) noexcept;
	~Thread() override;

	void Initialize(pantheon::Process *Proc, void *StartAddr, void *ThreadData, pantheon::Thread::Priority Priority, BOOL UserMode);

	[[nodiscard]] Process *MyProc() const;
	[[nodiscard]] Thread::State MyState() const;
	[[nodiscard]] Thread::Priority MyPriority() const;

	[[nodiscard]] UINT64 TicksLeft() const;
	[[nodiscard]] UINT64 ThreadID() const;

	VOID AddTicks(UINT64 TickCount);
	VOID CountTick();
	VOID RefreshTicks();
	VOID SetTicks(UINT64 TickCount);

	VOID SetState(Thread::State State);
	VOID SetPriority(Thread::Priority Priority);

	CpuContext *GetRegisters();
	[[nodiscard]] pantheon::vmm::VirtualAddress GetThreadLocalAreaRegister() const { return this->LocalRegion; }

	Thread &operator=(const Thread &Other);
	Thread &operator=(Thread &&Other) noexcept;

	void SetProc(pantheon::Process *Proc);

	void BlockScheduling();
	void EnableScheduling();
	[[nodiscard]] BOOL Preempted() const;

	[[nodiscard]] BOOL End() const;
	Thread *Next();
	void SetNext(pantheon::Thread *Item);

	ThreadLocalRegion *GetThreadLocalArea();
	VOID SetupThreadLocalArea();

private:
	VOID SetEntryLocation(UINT64 IP, UINT64 SP, VOID* ThreadData);

private:
	UINT64 TID;

	CpuContext Registers;
	Process *ParentProcess;

	Thread::State CurState;
	Thread::Priority CurPriority;

	pantheon::Atomic<UINT64> PreemptCount;
	pantheon::Atomic<UINT64> RemainingTicks;

	void *KernelStackSpace;
	void *UserStackSpace;

	pantheon::vmm::VirtualAddress LocalRegion;

	static constexpr UINT64 InitialNumStackPages = 4;

	pantheon::Atomic<pantheon::Thread*> NextThread;
};

}

#endif