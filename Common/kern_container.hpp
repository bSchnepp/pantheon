#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_CONTAINER_HPP_
#define _KERN_CONTAINER_HPP_

typedef Optional<void*> (*AllocatorMallocFn)(UINT64);
typedef void (*AllocatorFreeFn)(void*);

template<typename T>
struct ArrayListIterator
{
	ArrayListIterator() : ArrayListIterator(nullptr){};
	ArrayListIterator(T *Ptr) : Loc(Ptr){};

	T &operator*() 
	{ 
		return *this->Loc; 
	}

	ArrayListIterator<T> &operator++()
	{
		this->Loc++;
		return *this;
	}

	friend bool operator==(const ArrayListIterator<T> &L, const ArrayListIterator<T> &R)
	{
		return L.Loc == R.Loc;
	}

	friend bool operator!=(const ArrayListIterator<T> &L, const ArrayListIterator<T> &R)
	{
		return !(L == R);
	}

	private:
		T *Loc;
};

template<typename T>
class ArrayList
{
public:
	ArrayList() : ArrayList(10){};

	ArrayList(UINT64 InitCount) : ArrayList(InitCount, BasicMalloc, BasicFree){};

	ArrayList(AllocatorMallocFn Malloc, AllocatorFreeFn Free) : ArrayList(10, Malloc, Free){};

	ArrayList(UINT64 InitCount, AllocatorMallocFn Malloc, AllocatorFreeFn Free)
	{
		this->Malloc = Malloc;
		this->Free = Free;

		this->SpaceCount = 0;
		this->EntryCount = 0;
		this->Content = nullptr;

		auto MaybeMem = this->Malloc(sizeof(T) * InitCount);
		if (MaybeMem.GetOkay() != FALSE)
		{
			this->Content = (T*)MaybeMem.GetValue();
			this->SpaceCount = InitCount;
			this->EntryCount = 0;
		}
	}

	ArrayList(ArrayList &&Other) noexcept
	{
		this->SpaceCount = Other.SpaceCount;
		this->EntryCount = Other.EntryCount;
		this->Content = Other.Content;

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		Other.Content = nullptr;
		Other.SpaceCount = 0;
		Other.EntryCount = 0;		
	}

	ArrayList(const ArrayList &Other) noexcept
	{
		*this = ArrayList<T>(1);
		this->Copy(Other);
	}

	~ArrayList()
	{
		if (this->Content)
		{
			this->Clear();
			this->Free(this->Content);
		}
	}

	ArrayList<T> &operator=(const ArrayList<T> &Other) noexcept
	{
		if (this == &Other)
		{
			return *this;
		}		
		this->Copy(Other);
		return *this;
	}

	ArrayList<T> &operator=(ArrayList<T> &&Other) noexcept
	{
		this->SpaceCount = Other.SpaceCount;
		this->EntryCount = Other.EntryCount;
		this->Content = Other.Content;

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		if (this != &Other)
		{
			Other.Content = nullptr;
			Other.SpaceCount = 0;
			Other.EntryCount = 0;
		}
		return *this;

	}

	void Move(ArrayList<T> &Other) noexcept
	{
		if (this == &Other)
		{
			return;
		}

		if (this->Content)
		{
			this->Clear();
			this->Free(this->Content);
		}

		this->SpaceCount = Other.SpaceCount;
		this->EntryCount = Other.EntryCount;
		this->Content = Other.Content;

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		Other.SpaceCount = 0;
		Other.EntryCount = 0;
		Other.Content = nullptr;
	}

	void Clear()
	{
		if (this->Content)
		{
			for (T &Item : *this)
			{
				Item.~T();
			}
			ClearBuffer((CHAR*)this->Content, sizeof(T) * this->SpaceCount);
		}
	}

	void Copy(const ArrayList<T> &Other) noexcept
	{
		if (this == &Other)
		{
			return;
		}

		this->Clear();
		if (this->SpaceCount < Other.EntryCount)
		{
			this->Free(this->Content);
			auto MaybeMem = Other.Malloc(Other.EntryCount * sizeof(T));
			if (MaybeMem.GetOkay())
			{
				this->Content = (T*)MaybeMem.GetValue();
			}
			this->SpaceCount = Other.SpaceCount;
		}
		this->EntryCount = Other.EntryCount;
		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		for (UINT64 Index = 0; Index < Other.EntryCount; ++Index)
		{
			this->Content[Index] = Other.Content[Index];
		}
	}

	T &operator[](UINT64 Index) noexcept
	{
		/* This is needed to silence a warning on divide by 0 */
		if (this->EntryCount == 0 || this->Content == nullptr)
		{
			static T DefaultValue;
			DefaultValue = T();
			return DefaultValue;
		}
		return this->Content[Index % this->EntryCount];
	}

	[[nodiscard]] Optional<T> Get(UINT64 Index) const
	{
		if (Index < EntryCount)
		{
			return Optional<T>(Content[Index]);
		}
		return Optional<T>();
	}

	[[nodiscard]] UINT64 Size() const
	{
		return this->EntryCount;
	}

	[[nodiscard]] UINT64 AllocSpace() const
	{
		return this->SpaceCount;
	}

	void Add(T NewItem)
	{
		if (this->Content == nullptr)
		{
			*this = ArrayList<T>(1);
		}

		if (EntryCount + 1 < SpaceCount)
		{
			this->Content[this->EntryCount] = NewItem;
			this->EntryCount++;
			return;
		}
		
		/* If it couldn't fit, expand the storage */
		this->SpaceCount += 10;
		auto MaybeMem = this->Malloc(sizeof(T) * this->SpaceCount);
		if (MaybeMem.GetOkay())
		{
			T* NewContent = (T*)MaybeMem.GetValue();
			for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
			{
				T &Current = this->Content[Index];
				NewContent[Index] = Current;
			}

			for (T &Item : *this)
			{
				Item.~T();
			}
			this->Free(this->Content);
			this->Content = NewContent;
			this->Add(NewItem);
		}
	}

	void Delete(UINT64 Index)
	{
		if (Index >= this->EntryCount)
		{
			return;
		}

		auto MaybeMem = this->Malloc(sizeof(T) * this->SpaceCount);
		if (MaybeMem.GetOkay())
		{
			T* NewContent = (T*)MaybeMem.GetValue();
			UINT64 NewIndex = 0;
			for (UINT64 OrigIndex = 0; OrigIndex < this->EntryCount; ++OrigIndex)
			{
				NewContent[NewIndex] = this->Content[OrigIndex];
				if (OrigIndex == Index)
				{
					continue;
				}
				NewIndex++;
			}
			this->Clear();
			this->Free(this->Content);
			this->Content = NewContent;
		}
	}

	ArrayListIterator<T> begin()
	{
		if (this->EntryCount == 0)
		{
			return ArrayListIterator<T>();
		}
		return ArrayListIterator<T>(this->Content);
	}

	ArrayListIterator<T> end()
	{
		if (this->EntryCount == 0)
		{
			return ArrayListIterator<T>();
		}
		return ArrayListIterator<T>(&(this->Content[this->EntryCount]));
	}

	[[nodiscard]] BOOL Contains(T Item) const
	{
		/* NYI */
		PANTHEON_UNUSED(Item);
		return FALSE;
	}

private:
	AllocatorMallocFn Malloc;
	AllocatorFreeFn Free;

	UINT64 SpaceCount;
	UINT64 EntryCount;
	T *Content;
};


#endif