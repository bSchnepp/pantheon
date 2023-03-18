#include <arch.hpp>

#include "kern_spinlock.hpp"
#include "kern_datatypes.hpp"
#include "Proc/kern_cpu.hpp"

#include <kern_runtime.hpp>

pantheon::Spinlock::Spinlock() : pantheon::Spinlock::Spinlock("lock")
{
}

pantheon::Spinlock::Spinlock(const char *Name)
{
	this->DebugName = Name;
	this->CoreNo = -1;
	this->Locked = FALSE;
}

pantheon::Spinlock::~Spinlock()
{

}

[[nodiscard]] BOOL pantheon::Spinlock::IsHolding() const
{
	return (this->Locked && this->CoreNo == pantheon::CPU::GetProcessorNumber());
}

void pantheon::Spinlock::Acquire()
{
	OBJECT_SELF_ASSERT();

	if (this->DebugName == nullptr)
	{
		StopErrorFmt("Bad Spinlock: was nullptr\n");
	}
	
	pantheon::CPU::PUSHI();
	if (this->IsHolding())
	{
		StopErrorFmt("Spinlock: %s (source %p), trying to acquire when already held", this->DebugName, this);
	}

	for (;;)
	{
		if (__sync_lock_test_and_set(&this->Locked, TRUE) == FALSE)
		{
			break;
		}
		while (__atomic_load_n(&this->Locked, __ATOMIC_RELAXED))
		{
			pantheon::CPU::PAUSE();
		}
	}
	this->CoreNo = pantheon::CPU::GetProcessorNumber();
	__sync_synchronize();
}

BOOL pantheon::Spinlock::TryAcquire()
{
	OBJECT_SELF_ASSERT();

	if (this->DebugName == nullptr)
	{
		StopErrorFmt("Bad Spinlock: was nullptr\n");
	}
	
	pantheon::CPU::PUSHI();
	if (this->IsHolding())
	{
		StopErrorFmt("Spinlock: %s (source %p), trying to (try) acquire when already held", this->DebugName, this);
	}

	if (__sync_lock_test_and_set(&this->Locked, TRUE) == FALSE)
	{
		this->CoreNo = pantheon::CPU::GetProcessorNumber();
		__sync_synchronize();
		return TRUE;
	}
	
	while (__atomic_load_n(&this->Locked, __ATOMIC_RELAXED))
	{
		pantheon::CPU::PAUSE();
	}	
	__sync_synchronize();
	pantheon::CPU::POPI();
	return FALSE;
}

void pantheon::Spinlock::Release()
{
	OBJECT_SELF_ASSERT();
	
	__sync_synchronize();
	if (!this->IsHolding())
	{
		StopErrorFmt("Spinlock: %s (source %p), trying to release when not held (holder is %hd)\n", this->DebugName, this, this->Holder());
	}
	this->CoreNo = -1;
	__sync_synchronize();
	__sync_lock_release(&this->Locked);
	pantheon::CPU::POPI();
	__sync_synchronize();
	
}

/**
 * \~english @brief Checks if this given lock is already acquired.
 * \~english @details Returns true if and only if the lock is held.
 * 
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
BOOL pantheon::Spinlock::IsLocked() const
{
	return this->Locked == TRUE;
}

/**
 * \~english @brief Gets the current holder of this lock, if it is held.
 * \~english @details Returns -1 is this lock isn't held, otherwise gets the core number.
 * 
 * \~english @author Brian Schnepp
 */
[[nodiscard]] 
INT16 pantheon::Spinlock::Holder() const
{
	return (this->Locked ? this->CoreNo : static_cast<INT16>(-1));
}