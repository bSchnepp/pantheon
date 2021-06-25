#include "kern_spinlock.hpp"
#include "kern_datatypes.hpp"

pantheon::Spinlock::Spinlock()
{
	this->Locked = false;
}

pantheon::Spinlock::~Spinlock()
{

}

void pantheon::Spinlock::Acquire()
{
	asm volatile ("" ::: "memory");
	while (__sync_val_compare_and_swap(&(this->Locked), FALSE, TRUE) == TRUE)
	{

	}
	asm volatile ("" ::: "memory");
}

void pantheon::Spinlock::Release()
{
	asm volatile ("" ::: "memory");
	if (this->Locked == FALSE)
	{
		return;
	}
	while (__sync_val_compare_and_swap(&(this->Locked), TRUE, FALSE) == FALSE)
	{

	}
	asm volatile ("" ::: "memory");
}
