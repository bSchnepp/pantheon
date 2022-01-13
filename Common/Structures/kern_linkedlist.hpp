#include <kern_datatypes.hpp>


#ifndef _KERN_LINKEDLIST_HPP_
#define _KERN_LINKEDLIST_HPP_

namespace pantheon
{

template <typename T>
class LinkedList
{
public:
	template<typename K>
	struct LinkedListIterator;

	LinkedList() : LinkedList(nullptr, nullptr)
	{
	}

	LinkedList(T *Value) : LinkedList(nullptr, Value)
	{
	}

	LinkedList(LinkedList<T> *Next, T *Value)
	{
		this->NextInList = Next;
		this->Value = Value;
	}

	[[nodiscard]] T *GetValue() const
	{ 
		return this->Value; 
	}

	[[nodiscard]] LinkedList<T> *GetNext() const
	{ 
		return this->NextInList; 
	}

	[[nodiscard]] UINT64 Size() const 
	{
		UINT64 Count = 0;
		for (const LinkedList<T> *Cur = this; Cur != nullptr; Cur = Cur->NextInList)
		{
			Count++;
		}
		return Count;
	}

	[[nodiscard]] BOOL End() const
	{
		return this->NextInList == nullptr;
	}

	void Append(LinkedList<T> *Next)
	{
		for (LinkedList<T> *Iter = this; Iter != nullptr; Iter = Iter->NextInList)
		{
			if (Iter->End())
			{
				Iter->NextInList = Next;
				return;
			}

			if (Iter->Value == nullptr)
			{
				LinkedList<T> *NextEntry = Iter->NextInList;
				*Iter = *Next;
				Iter->NextInList = NextEntry;
				return;
			}
		}
	}

	[[nodiscard]] LinkedListIterator<T> begin() const
	{
		return LinkedListIterator<T>(this);
	}

	[[nodiscard]] LinkedListIterator<T> end() const
	{
		return LinkedListIterator<T>(nullptr);
	}

	static pantheon::LinkedList<T> *CreateEntry(T *Item)
	{
		Optional<void*> TryAlloc = BasicMalloc(sizeof(LinkedList<T>));
		if (TryAlloc.GetOkay())
		{
			LinkedList<T> *Res = static_cast<LinkedList<T>*>(TryAlloc());
			Res->Value = Item;
			Res->NextInList = nullptr;
			return Res;
		}
		return nullptr;
	}

	template<typename K>
	struct LinkedListIterator
	{
		LinkedListIterator() : LinkedListIterator(nullptr){};
		LinkedListIterator(const LinkedList<K> *Ptr) : Loc(Ptr){};

		K &operator*() 
		{ 
			return *(this->Loc->Value); 
		}

		LinkedListIterator<K> &operator++()
		{
			if (this->Loc)
			{
				this->Loc = this->Loc->NextInList;
			}
			return *this;
		}

		friend bool operator==(const LinkedListIterator<K> &L, const LinkedListIterator<K> &R)
		{
			return L.Loc == R.Loc;
		}

		friend bool operator!=(const LinkedListIterator<K> &L, const LinkedListIterator<K> &R)
		{
			return !(L == R);
		}

	private:
		const LinkedList<K> *Loc;
	};

private:
	pantheon::LinkedList<T> *NextInList;
	T* Value;
};


}
#endif