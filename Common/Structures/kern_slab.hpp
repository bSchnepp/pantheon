#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_SLAB_HPP_
#define _KERN_SLAB_HPP_

namespace pantheon::mm
{


typedef void (*AllocFn)(UINT64);
typedef void (*DeallocFn)(UINT64);

template<typename T>
class SlabCache
{
public:
	SlabCache(VOID *Area)
	{
		this->Used = 0;
		this->Bitmask = 0;
		this->Area = Area;
	}

	~SlabCache()
	{

	}

	BOOL InRange(T *Ptr)
	{
		/* Check if this is in range */
		UINT64 PtrRaw = static_cast<UINT64>(Ptr);
		UINT64 Base = static_cast<UINT64>(this->Area);
		UINT64 Max = Base + (64 * sizeof(T));
		if (PtrRaw < Base || PtrRaw > Max)
		{
			return FALSE;
		}
		return TRUE;
	}

	T *Allocate()
	{
		/* TODO: O(1) allocation with free list */
		UINT64 Count = 0;
		UINT64 CurMask = this->Bitmask;
		while (CurMask & 0x01 != 0)
		{
			CurMask >>= 1;
			Count++;
		}

		if (Count < 64)
		{
			return &(this->Area[Count]);
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
		for (UINT8 Area = 0; Area < 64; ++Area)
		{
			UINT64 BasePtr = Base + (Area * sizeof(T));
			if (BasePtr == PtrRaw)
			{
				this->Bitmask &= ~(1 << Area);
				return;
			}
		}
	}

	[[nodiscard]] BOOL Empty() const { return (this->Bitmask) == 0; }
	[[nodiscard]] BOOL Full() const { return (~this->Bitmask) == 0; }

private:
	UINT8 Used;
	UINT64 Bitmask;
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
		return nullptr;
	}

private:
	CacheList<T> *Caches;
};

}

#endif