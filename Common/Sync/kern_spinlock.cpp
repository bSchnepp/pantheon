#include <arch.hpp>

#include "kern_spinlock.hpp"
#include "kern_datatypes.hpp"
#include "Proc/kern_cpu.hpp"

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
	if (this->DebugName == nullptr)
	{
		StopError("bad spinlock");
	}
	
	pantheon::CPU::PUSHI();
	if (this->IsHolding())
	{
		StopError(this->DebugName, this);
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

void pantheon::Spinlock::Release()
{
	if (!this->IsHolding())
	{
		pantheon::StopError(this->DebugName, this);
	}
	this->CoreNo = -1;
	__sync_synchronize();
	__sync_lock_release(&this->Locked);
	pantheon::CPU::POPI();
	
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
 * \~english @details Returns 0 is this lock isn't held, otherwise gets the core number.
 * 
 * \~english @author Brian Schnepp
 */
[[nodiscard]] 
UINT8 pantheon::Spinlock::Holder() const
{
	return (this->Locked ? this->CoreNo : 0);
}