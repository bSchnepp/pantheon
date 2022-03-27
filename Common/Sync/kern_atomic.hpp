#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_ATOMIC_HPP_
#define _KERN_ATOMIC_HPP_

namespace pantheon
{

template<typename T>
class Atomic
{

public:
	FORCE_INLINE Atomic()
	{
		this->Content = T();
	}

	FORCE_INLINE Atomic(T Item)
	{
		this->Store(Item);
	}

	FORCE_INLINE T Load()
	{
		T RetVal;
		#ifndef ONLY_TESTS
		__atomic_load(&(this->Content), &RetVal, __ATOMIC_SEQ_CST);
		#else
		RetVal = this->Content;
		#endif
		return RetVal;
	}

	FORCE_INLINE void Store(T Item)
	{
		#ifndef ONLY_TESTS
		__atomic_store(&(this->Content), &Item, __ATOMIC_SEQ_CST);
		#else
		this->Content = Item;
		#endif
	}

	bool operator==(const Atomic<T> &Other)
	{
		return this->Load() == Other.Load();
	}

	void operator=(const T &Thing)
	{
		this->Store(Thing);
	}

	void operator=(T &&Thing)
	{
		this->Store(Thing);
	}

private:
	T Content;

};

}

#endif