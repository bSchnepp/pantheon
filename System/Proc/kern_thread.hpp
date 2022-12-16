#include <arch.hpp>

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
	enum LocalRegionFlag
	{
		FREE = (0 << 0),
		INTERRUPTED = (1 << 0),
	};

	struct LocalRegion
	{
		public:
			/* 4096 bytes each for communication */
			UINT32 BufferArea[0x1000 / sizeof(UINT32)];
			LocalRegionFlag Flags;
	};

public:
	typedef enum State
	{
		STATE_DEAD,
		STATE_INIT,
		STATE_RUNNING,
		STATE_WAITING,
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

	VOID SetEntryLocation(UINT64 IP, UINT64 SP, VOID* ThreadData);

	CpuContext *GetRegisters();
	void SetUserStackAddr(UINT64 Addr);
	void SetKernelStackAddr(UINT64 Addr);

	Thread &operator=(const Thread &Other);
	Thread &operator=(Thread &&Other) noexcept;

	void SetProc(pantheon::Process *Proc);

	void BlockScheduling();
	void EnableScheduling();
	[[nodiscard]] BOOL Preempted() const;

	[[nodiscard]] BOOL End() const;
	Thread *Next();
	void SetNext(pantheon::Thread *Item);

	void SignalThreadLocalArea() { this->ThreadLocalArea.Flags = Thread::LocalRegionFlag::INTERRUPTED; }
	UINT32 *GetThreadLocalArea() { return this->ThreadLocalArea.BufferArea; }
	void DesignalThreadLocalArea() { this->ThreadLocalArea.Flags = Thread::LocalRegionFlag::FREE; }

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
	LocalRegion ThreadLocalArea;

	static constexpr UINT64 InitialNumStackPages = 4;

	pantheon::Atomic<pantheon::Thread*> NextThread;
};

}

#endif