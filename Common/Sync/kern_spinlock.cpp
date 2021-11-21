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
	this->CoreNo = 0;
	this->Locked = false;	
}

pantheon::Spinlock::~Spinlock()
{

}

[[nodiscard]] BOOL pantheon::Spinlock::IsHolding() const
{
	__sync_synchronize();
	UINT8 Core = pantheon::CPU::GetProcessorNumber();
	UINT8 Lock = this->Locked;
	if (Core == this->CoreNo && Lock)
	{
		return TRUE;
	}
	return FALSE;
	__sync_synchronize();
}

void pantheon::Spinlock::Acquire()
{
	pantheon::CPU::PUSHI();

	for (;;)
	{
		if (this->IsHolding())
		{
			continue;
		}
		if (__sync_lock_test_and_set(&this->Locked, TRUE) == FALSE)
		{
			break;
		}
		pantheon::Sync::DSBSY();
		pantheon::Sync::ISB();
	}
	this->CoreNo = pantheon::CPU::GetProcessorNumber();
	__sync_synchronize();
}

void pantheon::Spinlock::Release()
{
	__sync_synchronize();
	if (!this->Locked)
	{
		pantheon::StopError(this->DebugName, this);
	}

	if (this->Holder() != pantheon::CPU::GetProcessorNumber())
	{
		pantheon::StopError(this->DebugName, this);
	}
	
	this->CoreNo = 0;
	__sync_synchronize();
	__sync_lock_release(&this->Locked);
	pantheon::CPU::POPI();
	pantheon::Sync::DSBSY();
	pantheon::Sync::ISB();	
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