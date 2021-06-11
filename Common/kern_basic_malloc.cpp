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

typedef UINT64 BlockHeader;
static constexpr UINT64 HeapSpace = 2 * 1024 * 1024;

static constexpr UINT64 MinBlockSize = sizeof(FreeList) + sizeof(BlockHeader);

alignas(32) static char BasicMemory[HeapSpace];

COMPILER_ASSERT(sizeof(BlockHeader) == sizeof(UINT64));

template<typename T>
static T Max(T L, T R)
{
	return (L > R) ? L : R;
}

static constexpr UINT64 Align(UINT64 Amt, UINT8 Align)
{
	return ~(Align - 1) & (((Amt) + (Align - 1)));
}

static char *GetHeader(char *Loc)
{
	return (Loc - sizeof(BlockHeader));
}

static UINT64 GetSize(char *Loc)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	return ((*Reinterp) & ~0x01);
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
static void *CurrentArea = nullptr;
static UINT64 CurrentSize = 0;

static BOOL InitMemoryOkay = FALSE;

static void UnlinkFreeList(FreeList *Current)
{
	if (GlobalFreeList == Current)
	{
		GlobalFreeList = GlobalFreeList->Next;
		return;
	}

	if (Current->Prev)
	{
		Current->Prev->Next = Current->Next;
	}

	if (Current->Next)
	{
		Current->Next->Prev = Current->Prev;
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
	CurrentArea = nullptr;
	CurrentSize = 0;

	for (char &Item : BasicMemory)
	{
		Item = '\0';
	}

	GlobalFreeList = reinterpret_cast<FreeList*>(BasicMemory + sizeof(BlockHeader));
	SetSizeAlloc(GetHeader((char*)GlobalFreeList), FALSE, HeapSpace - MinBlockSize);
	InitMemoryOkay = TRUE;
}

static pantheon::Spinlock AllocLock;


void CreateExplicitEntry(VOID *Addr)
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
 * should not be relied upon in any way.
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
	AllocLock.Acquire();
	InitBasicMemory();
	UINT64 Amount = Max(Align(Amt, (sizeof(BlockHeader))), MinBlockSize);

	UINT64 BlockTotal = 0;

	/* Look through the explicit free list for any space. */
	for (FreeList *Indexer = GlobalFreeList; 
		Indexer != nullptr && CurrentSize < HeapSpace - MinBlockSize; 
		Indexer = Indexer->Next)
	{
		UINT64 CurSize = GetSize(GetHeader((char*)Indexer));
		if (BlockTotal + Amount >= HeapSpace)
		{
			break;
		}
		BlockTotal += CurSize;
		if (CurSize - MinBlockSize >= Amount)
		{
			FreeList *Current = static_cast<FreeList*>(Indexer);

			UINT64 Diff = CurSize - Amount;
			SetSizeAlloc(GetHeader((char*)Indexer), TRUE, Amount);
			UnlinkFreeList(Current);

			char *Next = NextBlock((char*)(Indexer));
			SetSizeAlloc(GetHeader(Next), FALSE, Diff);
			CreateExplicitEntry(Next);

			AllocLock.Release();
			CurrentSize += Amount;	
			return Optional<void*>(Indexer);
		}
	}
	AllocLock.Release();
	return Optional<void*>();
}

void BasicFree(void *Addr)
{
	AllocLock.Acquire();
	
	/* Mark the block itself as free */
	CHAR *Header = GetHeader((CHAR*)Addr);
	UINT64 Size = GetSize(Header);
	SetSizeAlloc(Header, FALSE, Size);

	/* Append it to the free list. TODO: Coalescing! */
	CreateExplicitEntry(Addr);

	AllocLock.Release();
}