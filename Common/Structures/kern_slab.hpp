#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_SLAB_HPP_
#define _KERN_SLAB_HPP_

namespace pantheon::mm
{

typedef void (*AllocFn)(UINT64);
typedef void (*DeallocFn)(void*);

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

	FORCE_INLINE SlabCache()
	{
		this->Size = 0;
		this->Used = 0;
		this->FreeList = nullptr;
		this->Area = nullptr;
	}

	FORCE_INLINE SlabCache(VOID *Area, UINT16 Count = 64)
	{
		this->Area = reinterpret_cast<T*>(Area);
		#if POISON_MEMORY
			SetBufferBytes((char*)this->Area, 0xDF, Count * sizeof(T));
		#else
			ClearBuffer((char*)this->Area, Count * sizeof(T));
		#endif
		this->Size = Count;
		this->Used = 0;
		this->FreeList = nullptr;

		UINT64 BaseAddr = (UINT64)Area;
		UINT16 Index = 0;
		for (Index = 0; Index < Count - 1; Index++)
		{
			SlabNext<T>* Current = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * Index));
			SlabNext<T>* Next = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * (Index + 1)));
			Current->Next = Next;
		}
		SlabNext<T>* Last = reinterpret_cast<SlabNext<T>*>(BaseAddr + (sizeof(T) * Index));
		Last->Next = nullptr;
		this->FreeList = reinterpret_cast<SlabNext<T>*>(this->Area);
	}

	FORCE_INLINE ~SlabCache()
	{

	}

	FORCE_INLINE BOOL InRange(T *Ptr)
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

	FORCE_INLINE T *Allocate()
	{
		if (this->FreeList != nullptr)
		{
			/* Note this causes a warning, that Next may be
			 * undefined. Intuition says this is wrong, since
			 * the free list can only be manipulated by Allocate
			 * or Deallocate (and if it's smashed in memory,
			 * we have bigger problems), so it's always either
			 * valid or nullptr. We should fuzz this.
			 */
			SlabNext<T> *Current = this->FreeList;
			SlabNext<T> *Next = Current->Next;
			T* NewArea = reinterpret_cast<T*>(Current);
			this->FreeList = Next;
			this->Used++;
			*NewArea = T();
			return NewArea;
		}
		return nullptr;
	}

	FORCE_INLINE T *AllocateNoCtor()
	{
		if (this->FreeList != nullptr)
		{
			/* Note this causes a warning, that Next may be
			 * undefined. Intuition says this is wrong, since
			 * the free list can only be manipulated by Allocate
			 * or Deallocate (and if it's smashed in memory,
			 * we have bigger problems), so it's always either
			 * valid or nullptr. We should fuzz this.
			 */
			SlabNext<T> *Current = this->FreeList;
			SlabNext<T> *Next = Current->Next;
			T* NewArea = reinterpret_cast<T*>(Current);
			this->FreeList = Next;
			this->Used++;
			return NewArea;
		}
		return nullptr;
	}

	FORCE_INLINE void Deallocate(T *Ptr)
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
				this->Used--;
				return;
			}
		}
	}

	[[nodiscard]] UINT16 SlabCount() const { return this->Size; }
	[[nodiscard]] UINT16 SpaceLeft() const { return this->Size - Used; }
	[[nodiscard]] BOOL Empty() const { return this->SpaceLeft() == this->Size; }
	[[nodiscard]] BOOL Full() const { return this->FreeList == nullptr; }

private:
	UINT16 Used;
	UINT16 Size;
	SlabNext<T> *FreeList;
	T *Area;
};

template<typename T>
struct CacheList
{
	SlabCache<T> *Current;
	CacheList<T> *Next;
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