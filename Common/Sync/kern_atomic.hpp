#include <kern_datatypes.hpp>

#ifndef _KERN_ATOMIC_HPP_
#define _KERN_ATOMIC_HPP_

namespace pantheon
{

template<typename T>
class Atomic
{

public:
	Atomic() = default;

	Atomic(T Item)
	{
		this->Store(Item);
	}

	T Load()
	{
		T RetVal;
		__atomic_load(&(this->Content), &RetVal, __ATOMIC_SEQ_CST);
		return RetVal;
	}

	void Store(T Item)
	{
		__atomic_store(&(this->Content), &Item, __ATOMIC_SEQ_CST);		
	}

private:
	T Content;

};

}

#endif