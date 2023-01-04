#include <kern_datatypes.hpp>
#include <Common/Sync/kern_spinlock.hpp>
#include <Common/Structures/kern_slab.hpp>

#ifndef _KERN_ALLOCATABLE_HPP_
#define _KERN_ALLOCATABLE_HPP_

namespace pantheon
{

template<typename T, UINT64 Count>
class Allocatable
{
public:
	Allocatable() = default;
	~Allocatable() = default;

	static void Init()
	{
		AllocSpinlock = pantheon::Spinlock("Allocatable Lock");
		Allocator = pantheon::mm::SlabCache<T>(Items, Count);
	}

	static T *Create()
	{
		T *Ret = nullptr;
		AllocSpinlock.Acquire();
		Ret = Allocator.Allocate();
		AllocSpinlock.Release();
		return Ret;
	}

	static void Destroy(T *Item)
	{
		AllocSpinlock.Acquire();
		Allocator.Deallocate(Item);
		AllocSpinlock.Release();
	}


private:
	inline static pantheon::Spinlock AllocSpinlock;

	/* TODO: Use SlabAllocator instead! */

	inline static T Items[Count];
	inline static pantheon::mm::SlabCache<T> Allocator;

};


}

#endif