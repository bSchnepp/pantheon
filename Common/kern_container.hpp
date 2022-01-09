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
			#if POISON_MEMORY
				SetBufferBytes((char*)this->Content, 0xDF, InitCount * sizeof(T));
			#endif
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

	~ArrayList()
	{
		if (this->Content)
		{
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

	void Copy(const ArrayList<T> &Other) noexcept
	{
		if (this == &Other)
		{
			return;
		}
		
		if (this->Content && this->Free)
		{
			this->Free(this->Content);

			this->SpaceCount = 0;
			this->EntryCount = 0;
			this->Content = nullptr;
		}

		this->Malloc = Other.Malloc;
		this->Free = Other.Free;

		auto MaybeMem = this->Malloc(Other.EntryCount * sizeof(T));
		if (MaybeMem.GetOkay())
		{
			T* NewArea = (T*)MaybeMem.GetValue();
			#if POISON_MEMORY
				SetBufferBytes((char*)NewArea, 0xDF, Other.EntryCount * sizeof(T));
			#endif			
			for (UINT64 Index = 0; Index < Other.EntryCount; ++Index)
			{
				NewArea[Index] = Other.Content[Index];
			}
			this->Content = NewArea;
			this->SpaceCount = Other.SpaceCount;
			this->EntryCount = Other.EntryCount;
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
		if (EntryCount + 1 < SpaceCount)
		{
			this->Content[this->EntryCount] = NewItem;
			this->EntryCount++;
			return;
		}
		
		/* If it couldn't fit, expand the storage */
		this->SpaceCount *= 2;
		this->SpaceCount++;
		auto MaybeMem = this->Malloc(sizeof(T) * this->SpaceCount);
		if (MaybeMem.GetOkay())
		{
			T* NewContent = (T*)MaybeMem.GetValue();
			for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
			{
				T &Current = this->Content[Index];
				NewContent[Index] = Current;
			}
			#if POISON_MEMORY
				SetBufferBytes((char*)this->Content, 0xDF, this->SpaceCount * sizeof(T));
			#endif			
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
			this->Free(this->Content);
			this->Content = NewContent;
		}
	}

	[[nodiscard]] ArrayListIterator<T> begin() const
	{
		if (this->EntryCount == 0)
		{
			return ArrayListIterator<T>();
		}
		return ArrayListIterator<T>(this->Content);
	}

	[[nodiscard]] ArrayListIterator<T> end() const
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