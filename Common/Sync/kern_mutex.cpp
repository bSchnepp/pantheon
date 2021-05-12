#include "kern_mutex.hpp"
#include "kern_datatypes.hpp"

pantheon::Mutex::Mutex()
{
	this->Locked = false;
}

pantheon::Mutex::~Mutex()
{
	
}

void pantheon::Mutex::Acquire()
{
	asm volatile ("" ::: "memory");
	while (__sync_val_compare_and_swap(&(this->Locked), FALSE, TRUE))
	{

	}
	asm volatile ("" ::: "memory");
}

void pantheon::Mutex::Release()
{
	asm volatile ("" ::: "memory");
	while (__sync_val_compare_and_swap(&(this->Locked), TRUE, FALSE))
	{

	}
	asm volatile ("" ::: "memory");
}
