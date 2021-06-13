#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_CONTAINER_HPP_
#define _KERN_CONTAINER_HPP_

typedef Optional<void*> (*AllocatorMallocFn)(UINT64);
typedef void (*AllocatorFreeFn)(void*);

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

	void Move(ArrayList<T> &Other) noexcept
	{
		this->SpaceCount = Other.SpaceCount;
		this->EntryCount = Other.EntryCount;
		this->Content = Other.Content;

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		if (this != &Other)
		{
			Other.Clear();
			Other.SpaceCount = 0;
			Other.EntryCount = 0;
			Other.Content = nullptr;
		}
	}

	void Clear()
	{
		for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
		{
			this->Content[Index].~T();
		}
	}

	void Copy(const ArrayList<T> &Other) noexcept
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

		this->SpaceCount = 0;
		this->EntryCount = 0;
		this->Content = nullptr;

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		auto MaybeMem = this->Malloc(Other.EntryCount);
		if (MaybeMem.GetOkay())
		{
			this->Content = (T*)MaybeMem.GetValue();
			this->SpaceCount = Other.SpaceCount;
			this->EntryCount = Other.EntryCount;
			for (UINT64 Index = 0; Index < Other.EntryCount; ++Index)
			{
				this->Content[Index] = Other.Content[Index];
			}
		}
	}

	T &operator[](UINT64 Index) noexcept
	{
		/* This is needed to silence a warning on divide by 0 */
		if (this->EntryCount == 0)
		{
			return *(this->Content);
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
		if (EntryCount + 1 < SpaceCount)
		{
			this->Content[this->EntryCount] = NewItem;
			this->EntryCount++;
			return;
		}
		
		this->SpaceCount *= 4;
		auto MaybeMem = this->Malloc(sizeof(T) * this->SpaceCount);
		if (MaybeMem.GetOkay())
		{
			T* NewContent = (T*)MaybeMem.GetValue();
			for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
			{
				T &Current = this->Content[Index];
				NewContent[Index] = Current;
			}
			this->Clear();
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
			for (UINT64 SIndex = 0; SIndex < Index; ++SIndex)
			{
				NewContent[SIndex] = this->Content[SIndex];
			}
			for (UINT64 SIndex = Index + 1; SIndex < this->EntryCount; ++SIndex)
			{
				NewContent[SIndex-1] = this->Content[SIndex];
			}
			this->Clear();
			this->Free(this->Content);
			this->Content = NewContent;
		}
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