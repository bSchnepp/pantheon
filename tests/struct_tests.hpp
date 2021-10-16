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
	for (UINT8 Index = 0; Index < 128; Index++)
	{
		ASSERT_NE(Entries.Allocate(), nullptr);
	}
	ASSERT_EQ(Entries.Allocate(), nullptr);
}

TEST(SlabAlloc, BasicSlabEmpty)
{
	void *Area = malloc(sizeof(SEntry) * 128);
	SEntry *AreaAreas[128];

	pantheon::mm::SlabCache<SEntry> Entries(Area, 128);
	for (auto &AreaA : AreaAreas)
	{
		AreaA = (SEntry*)Entries.Allocate();
		ASSERT_NE(AreaA, nullptr);
	}

	ASSERT_EQ(Entries.Allocate(), nullptr);
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
}

#endif