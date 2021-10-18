#include <list>

#include <gtest/gtest.h>

#include <Common/Structures/kern_bitmap.hpp>
#include <Common/Structures/kern_slab.hpp>

#ifndef STRUCT_TESTS_HPP_
#define STRUCT_TESTS_HPP_

typedef struct SEntry
{
	UINT64 Item;
	UINT64 Item2;
}SEntry;

TEST(SlabAlloc, BasicSlabCache)
{
	void *Area = malloc(sizeof(SEntry) * 128);
	pantheon::mm::SlabCache<SEntry> Entries(Area, 128);
	ASSERT_TRUE(Entries.Empty());
	for (UINT8 Index = 0; Index < 128; Index++)
	{
		ASSERT_NE(Entries.Allocate(), nullptr);
	}
	ASSERT_EQ(Entries.Allocate(), nullptr);
	ASSERT_TRUE(Entries.Full());
	free(Area);
}

TEST(SlabAlloc, BasicSlabEmpty)
{
	void *Area = malloc(sizeof(SEntry) * 128);
	SEntry *AreaAreas[128];

	pantheon::mm::SlabCache<SEntry> Entries(Area, 128);
	ASSERT_EQ(Entries.SpaceLeft(), 128);
	for (auto &AreaA : AreaAreas)
	{
		AreaA = (SEntry*)Entries.Allocate();
		ASSERT_NE(AreaA, nullptr);
	}

	ASSERT_EQ(Entries.Allocate(), nullptr);
	ASSERT_EQ(Entries.SpaceLeft(), 0);
	for (auto &AreaA : AreaAreas)
	{
		Entries.Deallocate(AreaA);
		ASSERT_NE(AreaA, nullptr);
	}

	for (auto &AreaA : AreaAreas)
	{
		AreaA = (SEntry*)Entries.Allocate();
		ASSERT_NE(AreaA, nullptr);
	}
	free(Area);
}

TEST(SlabAlloc, SlabDeallocInvalid)
{
	constexpr UINT8 Size = 1;
	void *Area = malloc(sizeof(SEntry) * Size);
	pantheon::mm::SlabCache<SEntry> Entries(Area, Size);

	SEntry *First = Entries.Allocate();

	ASSERT_EQ(Entries.Allocate(), nullptr);

	char *Invalid = ((char*)First) + 0x02;
	Entries.Deallocate((SEntry*)Invalid);

	ASSERT_EQ(Entries.Allocate(), nullptr);
	Entries.Deallocate(First);
	ASSERT_NE(Entries.Allocate(), nullptr);
	free(Area);
}

typedef struct Page
{
	char Content[4096];
}Page;


TEST(SlabAlloc, SlabUseBigStruct)
{
	constexpr UINT16 Size = 512;
	void *Area = malloc(sizeof(Page) * Size);
	pantheon::mm::SlabCache<Page> Entries(Area, Size);

	std::list<Page*> Items;

	for (UINT16 Index = 0; Index < Size; ++Index)
	{
		Page *Item = Entries.Allocate();
		memset(Item, Size, 0xDD);
		Items.push_back(Item);
	}

	for (Page *Item : Items)
	{
		Entries.Deallocate(Item);
	}

	ASSERT_EQ(Entries.SpaceLeft(), Size);

	for (UINT16 Index = 0; Index < Size; ++Index)
	{
		ASSERT_NE(Entries.Allocate(), nullptr);
	}
	free(Area);
}

#endif