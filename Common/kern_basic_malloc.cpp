#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

typedef UINT64 BlockHeader;
static constexpr UINT64 HeapSpace = 2 * 1024 * 1024;
static char BasicMemory[HeapSpace];

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

static UINT8 GetAlloc(char *Loc)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	return ((*Reinterp) & 0x01);
}


/* Note that "Val != 0" is needed to guarantee either a 0 or 1 as output. */
static void SetAlloc(char *Loc, BOOL Val)
{
	char *NewLoc = GetHeader(Loc);
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(NewLoc);
	UINT64 Size = GetSize((char*)Reinterp);
	*Reinterp = (Size | (Val != 0));
}

static void SetSize(char *Loc, UINT64 Size)
{
	char *NewLoc = GetHeader(Loc);
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(NewLoc);
	BOOL Used = GetAlloc((char*)Reinterp);
	*Reinterp = (Size | Used);
}

static void SetSizeAlloc(char *Loc, BOOL Used, UINT64 Size)
{
	BlockHeader *Reinterp = reinterpret_cast<BlockHeader*>(Loc);
	*Reinterp = Size | Used != 0;
}

static char *GetFooter(char *Loc)
{
	return Loc + GetSize(GetHeader(Loc)) - sizeof(BlockHeader);
}

static char *NextBlock(char *Loc)
{
	return Loc + GetSize(GetHeader(Loc));
}

static char *PrevBlock(char *Loc)
{
	return Loc - GetSize(GetHeader(Loc)) - sizeof(BlockHeader);
}

typedef struct FreeList
{
	struct FreeList *Prev;
	struct FreeList *Next;
}FreeList;

static void UnlinkFreeList(FreeList *Current)
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

static constexpr UINT64 MinBlockSize = sizeof(FreeList) + sizeof(BlockHeader);

static FreeList *GlobalFreeList = nullptr;
static void *CurrentArea = nullptr;
static UINT64 CurrentSize = 0;

static BOOL InitMemoryOkay = FALSE;

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

	for (UINT64 Index = 0; Index < HeapSpace; ++Index)
	{
		BasicMemory[Index] = 0;
	}

	GlobalFreeList = reinterpret_cast<FreeList*>(BasicMemory + sizeof(BlockHeader));
	SetSizeAlloc(GetHeader((char*)GlobalFreeList), FALSE, HeapSpace - MinBlockSize);
	SetSizeAlloc(GetFooter((char*)GlobalFreeList), FALSE, HeapSpace - MinBlockSize);
	InitMemoryOkay = TRUE;
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
			FreeList *NewItem = (FreeList*)(Next);

			NewItem->Prev = nullptr;
			NewItem->Next = GlobalFreeList;

			if (GlobalFreeList)
			{
				GlobalFreeList->Prev = NewItem;
			}
			GlobalFreeList = NewItem;
						
			return Optional<void*>(Indexer);
		}
	}
	return Optional<void*>();
}

void BasicFree(void *Addr)
{
	/* Do nothing for now... */
}