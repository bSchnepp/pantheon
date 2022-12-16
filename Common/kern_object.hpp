#include <stdatomic.h>
#include <kern_datatypes.hpp>
#include <Common/Sync/kern_atomic.hpp>

#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_OBJECT_HPP_
#define _KERN_OBJECT_HPP_

namespace pantheon
{

template<typename T, UINT64 Count = 512>
class Object : public Allocatable<T, Count>
{
	struct RefCount
	{
		atomic_ullong Counter = 0;
	};

public:
	UINT64 Open()
	{
		return atomic_fetch_add(&this->RefCounter.Counter, 1) + 1;
	}

	UINT64 Close()
	{
		UINT64 NewVal = atomic_fetch_add(&this->RefCounter.Counter, -1) - 1;
		if (NewVal <= 0)
		{
			Allocatable<T, Count>::Destroy((T*)this);
		}
		return NewVal;
	}

private:
	RefCount RefCounter;
};

}

#endif