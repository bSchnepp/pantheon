#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_SLAB_HPP_
#define _KERN_SLAB_HPP_

namespace pantheon::mm
{

typedef void (*AllocFn)(UINT64);
typedef void (*DeallocFn)(UINT64);

template<typename T>
struct SlabNext
{
	SlabNext<T> *Next;
};

template<typename T>
class SlabCache
{
public:
	static_assert(sizeof(T) >= sizeof(SlabNext<T>));

	SlabCache(VOID *Area, UINT16 Count = 64)
	{
		this->Area = reinterpret_cast<T*>(Area);
		this->Size = Count;

		UINT64 BaseAddr = (UINT64)Area;
		UINT16 Index = 0;
		for (Index = 0; Index < Count - 1; Index++)
		{
			SlabNext<T>* Current = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * Index));
			SlabNext<T>* Next = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * (Index + 1)));
			Current->Next = Next;
		}
		SlabNext<T>* Last = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * Index));
		this->FreeList = reinterpret_cast<SlabNext<T>*>(this->Area);
	}

	~SlabCache()
	{

	}

	BOOL InRange(T *Ptr)
	{
		/* Check if this is in range */
		UINT64 PtrRaw = reinterpret_cast<UINT64>(Ptr);
		UINT64 Base = reinterpret_cast<UINT64>(this->Area);
		UINT64 Max = Base + (this->Size * sizeof(T));
		if (PtrRaw < Base || PtrRaw > Max)
		{
			return FALSE;
		}
		return TRUE;
	}

	T *Allocate()
	{
		if (this->FreeList != nullptr)
		{
			T* NewArea = reinterpret_cast<T*>(this->FreeList);
			this->FreeList = this->FreeList->Next;
			return NewArea;
		}
		return nullptr;
	}

	void Deallocate(T *Ptr)
	{
		if (!InRange(Ptr))
		{
			return;
		}

		/* Mark the area as deallocated iff it's actually valid. */
		for (UINT16 Area = 0; Area < this->Size; ++Area)
		{
			UINT64 Base = (UINT64)this->Area;
			UINT64 BasePtr = Base + (Area * sizeof(T));
			if (BasePtr == (UINT64)Ptr)
			{
				SlabNext<T> *Next = reinterpret_cast<SlabNext<T>*>(Ptr);
				Next->Next = FreeList;
				this->FreeList = Next;
				return;
			}
		}
	}

	[[nodiscard]] BOOL Full() const { return this->FreeList != nullptr; }

private:
	UINT16 Size;
	SlabNext<T> *FreeList;
	T *Area;
};

template<typename T>
struct CacheList
{
	SlabCache<T> *Current;
	SlabCache<T> *Next;
};

template<typename T>
class SlabAllocator
{
public:
	SlabAllocator()
	{
		this->Caches = nullptr;
	}

	~SlabAllocator(){};

	T *Allocate()
	{
		CacheList<T> *Current = &Caches;
		while (Current != nullptr)
		{
			if (!Current->Current->Full())
			{
				return Current->Current->Allocate();
			}
		}
		/* TODO: Expand caches? */
		return nullptr;
	}

	void Deallocate(T *Value)
	{
		CacheList<T> *Current = &Caches;
		while (Current != nullptr)
		{
			if (Current->Current->InRange(Value))
			{
				Current->Current->Deallocate(Value);
			}
		}
	}

private:
	CacheList<T> *Caches;
};

}

#endif