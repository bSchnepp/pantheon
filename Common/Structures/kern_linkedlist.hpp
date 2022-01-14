#include <kern_datatypes.hpp>


#ifndef _KERN_LINKEDLIST_HPP_
#define _KERN_LINKEDLIST_HPP_

namespace pantheon
{

template <typename T>
class LinkedListItem
{
public:
	LinkedListItem() : LinkedListItem(nullptr, nullptr)
	{
	}

	LinkedListItem(T *Value) : LinkedListItem(nullptr, Value)
	{
	}

	LinkedListItem(LinkedListItem<T> *Next, T *Value)
	{
		this->NextInList = Next;
		this->Value = Value;
	}

	[[nodiscard]] T *GetValue() const
	{ 
		return this->Value; 
	}

	[[nodiscard]] LinkedListItem<T> *GetNext() const
	{ 
		return this->NextInList; 
	}

	void SetNext(LinkedListItem<T> *Next)
	{
		this->NextInList = Next;
	}

	[[nodiscard]] UINT64 Size() const 
	{
		UINT64 Count = 0;
		for (const LinkedListItem<T> *Cur = this; Cur != nullptr; Cur = Cur->GetNext())
		{
			Count++;
		}
		return Count;
	}

	[[nodiscard]] BOOL End() const
	{
		return this->NextInList == nullptr;
	}

	void Append(LinkedListItem<T> *Next)
	{
		for (LinkedListItem<T> *Iter = this; Iter != nullptr; Iter = Iter->NextInList)
		{
			if (Iter->End())
			{
				Iter->NextInList = Next;
				return;
			}

			if (Iter->Value == nullptr)
			{
				LinkedListItem<T> *NextEntry = Iter->NextInList;
				*Iter = *Next;
				Iter->NextInList = NextEntry;
				return;
			}
		}
	}

	static pantheon::LinkedListItem<T> *CreateEntry(T *Item)
	{
		Optional<void*> TryAlloc = BasicMalloc(sizeof(LinkedListItem<T>));
		if (TryAlloc.GetOkay())
		{
			LinkedListItem<T> *Res = static_cast<LinkedListItem<T>*>(TryAlloc());
			Res->Value = Item;
			Res->NextInList = nullptr;
			return Res;
		}
		return nullptr;
	}

	static void DestroyEntry(pantheon::LinkedListItem<T> *Item)
	{
		if (Item != nullptr)
		{
			BasicFree(Item);
		}
	}

private:
	pantheon::LinkedListItem<T> *NextInList;
	T* Value;
};

template<typename T>
class LinkedList
{
public:
	template<typename K>
	struct LinkedListIterator;

	LinkedList() : LinkedList(nullptr)
	{
	}

	LinkedList(LinkedListItem<T> *ListRoot) : Root(ListRoot)
	{
		/* Size is either 0 or 1, depending on if Root is valid. */
		this->Size = (Root != nullptr);
	}

	void PushFront(T *Item)
	{
		LinkedListItem<T> *NewEntry = LinkedListItem<T>::CreateEntry(Item);
		if (this->Root == nullptr)
		{
			Size = 1;
			Root = NewEntry;
			return;
		}

		NewEntry->SetNext(this->Root);
		this->Root = NewEntry;
		this->Size++;
	}

	T *PopFront()
	{
		LinkedListItem<T> *OldRoot = this->Root;
		this->Root = this->Root->GetNext();
		this->Size--;

		T *Item = OldRoot->GetValue();
		LinkedListItem<T>::DestroyEntry(OldRoot);
		return Item;
	}

	[[nodiscard]] LinkedListIterator<T> begin() const
	{
		return LinkedListIterator<T>(this->Root);
	}

	[[nodiscard]] LinkedListIterator<T> end() const
	{
		return LinkedListIterator<T>(nullptr);
	}

	[[nodiscard]] UINT64 GetSize() const
	{
		return this->Size;
	}

	template<typename K>
	struct LinkedListIterator
	{
		LinkedListIterator() : LinkedListIterator(nullptr){};
		LinkedListIterator(const LinkedListItem<K> *Ptr) : Loc(Ptr){};

		K &operator*() 
		{ 
			return *(this->Loc->GetValue()); 
		}

		LinkedListIterator<K> &operator++()
		{
			if (this->Loc)
			{
				this->Loc = this->Loc->GetNext();
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
		const LinkedListItem<K> *Loc;
	};

private:
	UINT64 Size;
	LinkedListItem<T> *Root;
};


}
#endif