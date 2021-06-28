#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>
#include <Sync/kern_spinlock.hpp>

typedef struct FreeList
{
	struct FreeList *Prev;
	struct FreeList *Next;
}FreeList;

/* Try to align to 32-bytes, so that allocations can be bitpacked nicely. */
typedef UINT64 BlockHeader;
static constexpr UINT64 HeapSpace = 6 * 1024 * 1024;
static constexpr UINT64 MinBlockSize = sizeof(FreeList) + (2 * sizeof(BlockHeader));
COMPILER_ASSERT(MinBlockSize == 32);

alignas(32) static char BasicMemory[HeapSpace];

COMPILER_ASSERT(sizeof(BlockHeader) == sizeof(UINT64));

static char *GetHeader(char *Loc)
{
	return (Loc - sizeof(BlockHeader));
}

static UINT64 GetSize(char *Loc)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	return ((*Reinterp) & ~0x01);
}

static UINT64 GetAlloc(char *Loc)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	return ((*Reinterp) & 0x01);
}

static void SetSizeAlloc(char *Loc, BOOL Used, UINT64 Size)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	*Reinterp = Size | (Used != 0);
}

static char *NextBlock(char *Loc)
{
	return Loc + GetSize(GetHeader(Loc));
}

static FreeList *GlobalFreeList = nullptr;

static BOOL InitMemoryOkay = FALSE;

static void UnlinkFreeList(FreeList *Current)
{
	if (GlobalFreeList == Current)
	{
		GlobalFreeList = GlobalFreeList->Next;
	}
	else
	{
		if (Current->Prev)
		{
			Current->Prev->Next = Current->Next;
		}

		if (Current->Next)
		{
			Current->Next->Prev = Current->Prev;
		}
	}
}



void InitBasicMemory()
{
	/*  lazilly handle basic malloc memory */
	if (InitMemoryOkay)
	{
		return;
	}

	/* Double check this, just to be sure. */
	GlobalFreeList = nullptr;

	for (char &Item : BasicMemory)
	{
		Item = '\0';
	}

	/* Mark the ends as in use, so coalescing doesn't become a problem. */
	SetSizeAlloc(((char*)(BasicMemory)), TRUE, sizeof(BlockHeader));
	SetSizeAlloc(((char*)(BasicMemory)) + sizeof(BlockHeader), FALSE, HeapSpace - 2 * sizeof(BlockHeader));
	SetSizeAlloc(((char*)(BasicMemory)) + HeapSpace - sizeof(BlockHeader), TRUE, sizeof(BlockHeader));

	GlobalFreeList = reinterpret_cast<FreeList*>(NextBlock(BasicMemory));
	GlobalFreeList->Next = nullptr;
	GlobalFreeList->Prev = nullptr;
	InitMemoryOkay = TRUE;
}

static pantheon::Spinlock AllocLock;


void LinkFreeList(VOID *Addr)
{
	FreeList *List = (FreeList*)(Addr);

	List->Next = nullptr;
	List->Prev = nullptr;

	if (GlobalFreeList)
	{
		GlobalFreeList->Prev = List;
	}
	List->Next = GlobalFreeList;
	GlobalFreeList = List;
}

/**
 * \~english @brief Tries to allocate a block of memory from a static heap.
 * \~english @details An attempt is made to allocate against a statically 
 * defined heap. The precise size of this heap should not be relied upon, but 
 * it should be at least the size of 512 pages of the underlying system.
 * 
 * This malloc implementation should generally be reserved for the booting stage,
 * in order to set up basic drivers or a more complex memory allocation system,
 * and should not be used by system drivers.
 * 
 * Internally, a classical memory management system with an explicit free list
 * and packed headers are used, aiming for a reasonable tradeoff for implementation
 * complexity and usability. As such, BasicFree() calls will actually mark
 * memory as free, and not allow for any memory leaks, unlike a more common
 * bump allocator.
 * 
 * The specific details of how this is done may change in the future, such as
 * changing to a more dynamic buddy allocation system, or to a slab allocator,
 * or to some blended variant thereof. Thus, the content of a claimed chunk
 * should not be relied upon in any way, after deallocation or before filling
 * with user-supplied data.
 * 
 * \~english @param[in] Amt The amount of memory, in bytes, to claim from the heap
 * \~english @return An Optional type containing a void* to the address to use, or
 * an Optional marked as false, indicating no free memory left.
 * \~english @author Brian Schnepp
 * 
 * @see https://en.wikipedia.org/wiki/Free_list
 */
Optional<void*> BasicMalloc(UINT64 Amt)
{
	/* If there's no space to allocate, don't even try. */
	if (Amt == 0 || Amt > HeapSpace)
	{
		return Optional<void*>();
	}
	
	AllocLock.Acquire();
	InitBasicMemory();
	UINT64 Amount = Max(Align(Amt, (sizeof(BlockHeader))), MinBlockSize);

	/* Look through the explicit free list for any space. */
	for (FreeList *Indexer = GlobalFreeList; 
		Indexer != nullptr; 
		Indexer = Indexer->Next)
	{
		UINT64 CurSize = GetSize(GetHeader((char*)Indexer));
		if (CurSize - MinBlockSize >= Amount)
		{
			FreeList *Current = static_cast<FreeList*>(Indexer);

			UINT64 Diff = CurSize - Amount;
			SetSizeAlloc(GetHeader((char*)Indexer), TRUE, Amount);
			UnlinkFreeList(Current);

			char *Next = NextBlock((char*)(Indexer));
			SetSizeAlloc(GetHeader(Next), FALSE, Diff);
			LinkFreeList(Next);

			AllocLock.Release();
			return Optional<void*>(Current);
		}
	}
	AllocLock.Release();
	return Optional<void*>();
}

void BasicFree(void *Addr)
{
	if (Addr == nullptr)
	{
		return;
	}

	AllocLock.Acquire();
	
	/* Ensure the current block is consistent, ie, no double frees */
	CHAR *ActualAddr = (CHAR*)Addr;
	if (GetAlloc(GetHeader((CHAR*)Addr)) == FALSE)
	{
		AllocLock.Release();
		return;
	}

	UINT64 Size = 0;
	/* TODO: Handle merging with other adjacent blocks as needed, ie,
	 * if malloc-related tests start failing. 
	 * 
	 * In the future, the allocator should be blended with some additional 
	 * layers, so that small allocations of 128 bytes or less can be cached 
	 * until theres no more cache space, helping improve performance and 
	 * maximizing throughput.
	 */

	/* Mark the current block as free. */
	UINT64 CurrentSize = GetSize(GetHeader((CHAR*)Addr));
	SetSizeAlloc(GetHeader((CHAR*)Addr), FALSE, CurrentSize);
	Size += CurrentSize;
	
	/* Merge with next block if possible... */
	CHAR *Next = NextBlock((CHAR*)Addr);
	if (GetAlloc(GetHeader(Next)) == FALSE)
	{
		UINT64 NextSize = GetSize(GetHeader(Next));
		SetSizeAlloc(GetHeader((CHAR*)Next), FALSE, NextSize);
		UnlinkFreeList((FreeList*)Next);
		Size += NextSize;
	}

	/* Final data */
	SetSizeAlloc(GetHeader(ActualAddr), FALSE, Size);

	/* Append it to the free list. */
	LinkFreeList(ActualAddr);
	AllocLock.Release();
}