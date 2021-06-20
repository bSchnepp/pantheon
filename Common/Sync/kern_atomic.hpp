#include <kern_datatypes.hpp>

#ifndef _KERN_ATOMIC_HPP_
#define _KERN_ATOMIC_HPP_

namespace pantheon
{

template<typename T>
class Atomic
{

public:
	Atomic()
	{
		this->Content = T();
	}

	Atomic(T Item)
	{
		this->Store(Item);
	}

	T Load()
	{
		T RetVal;
		#ifndef ONLY_TESTS
		__atomic_load(&(this->Content), &RetVal, __ATOMIC_SEQ_CST);
		#else
		RetVal = this->Content;
		#endif
		return RetVal;
	}

	void Store(T Item)
	{
		#ifndef ONLY_TESTS
		__atomic_store(&(this->Content), &Item, __ATOMIC_SEQ_CST);
		#else
		this->Content = Item;
		#endif
	}

private:
	T Content;

};

}

#endif