#include <arch.hpp>

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
	for (;;)
	{
		if (__atomic_exchange_n(&this->Locked, TRUE, __ATOMIC_ACQUIRE) == FALSE)
		{
			break;
		}
		while (__atomic_load_n(&this->Locked, __ATOMIC_RELAXED))
		{
			pantheon::CPU::PAUSE();
		}
	}
}

void pantheon::Spinlock::Release()
{
	__atomic_exchange_n(&this->Locked, FALSE, __ATOMIC_RELEASE);
}
