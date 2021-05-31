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
	ArrayList() : ArrayList(2){};

	ArrayList(UINT64 InitCount)
	{
		this->Malloc = BasicMalloc;
		this->Free = BasicFree;

		auto MaybeMem = this->Malloc(sizeof(T) * InitCount);
		if (MaybeMem.GetOkay() != FALSE)
		{
			this->Content = (T*)MaybeMem.GetValue();
			this->SpaceCount = InitCount;
			this->EntryCount = 0;
		}
	}

	Optional<T> operator[](UINT64 Index)
	{
		if (Index < EntryCount)
		{
			return Optional<T>(Content[Index]);
		}
		return Optional<T>();
	}

	T* Get(UINT64 Index)
	{
		return this->Content[Index];
	}

	UINT64 Size()
	{
		return this->EntryCount;
	}

	void Add(T& NewItem)
	{
		if (EntryCount + 1 < SpaceCount)
		{
			this->Content[this->EntryCount] = NewItem;
			this->EntryCount++;
		}
		else
		{
			this->SpaceCount *= 2;
			auto MaybeMem = this->Malloc(sizeof(T) * this->SpaceCount);
			if (MaybeMem.GetOkay() != FALSE)
			{
				T* NewContent = (T*)MaybeMem.GetValue();
				for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
				{
					NewContent[Index] = this->Content[Index];
				}
				this->Free(this->Content);
				this->Content = NewContent;
				this->Add(NewItem);
			}			
		}
	}

	void Delete(UINT64 Index)
	{
		/* NYI */
	}

	BOOL Contains(T& Item)
	{
		/* NYI */
	}

private:
	AllocatorMallocFn Malloc;
	AllocatorFreeFn Free;

	UINT64 SpaceCount;
	UINT64 EntryCount;
	T *Content;
};


#endif