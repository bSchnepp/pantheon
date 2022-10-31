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
	Atomic()
	{
		this->Content = T();
	}

	Atomic(T Item)
	{
		this->Store(Item);
	}

	~Atomic() = default;

	[[nodiscard]]
	T Load() const
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

	[[nodiscard]] 
	bool operator==(const Atomic<T> &Other) const
	{
		return this->Load() == Other.Load();
	}

	[[nodiscard]] 
	bool operator==(const T &Other) const
	{
		return this->Load() == Other;
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