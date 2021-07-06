#include <gtest/gtest.h>

#include <System/Memory/kern_physpaging.hpp>
#include <Common/Structures/kern_bitmap.hpp>

#ifndef STRUCT_TESTS_HPP_
#define STRUCT_TESTS_HPP_

TEST(Bitmap, BasicInit)
{
	pantheon::Bitmap B(16);
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		ASSERT_FALSE(B.Get(Index));
	}
}

TEST(Bitmap, BasicInitByte)
{
	pantheon::Bitmap B(16);
	for (UINT64 Index = 0; Index < 16 / 8; ++Index)
	{
		ASSERT_EQ(B.GetByte(Index), 0);
	}
}

TEST(Bitmap, BasicFill)
{
	pantheon::Bitmap B(16);
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		B.Set(Index, TRUE);
	}
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		ASSERT_TRUE(B.Get(Index));
	}
}

TEST(Bitmap, EveryOtherFill)
{
	pantheon::Bitmap B(16);
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		if (Index % 2)
		{
			B.Set(Index, TRUE);
		}
	}
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		if (Index % 2)
		{
			ASSERT_TRUE(B.Get(Index));
		}
		else
		{
			ASSERT_FALSE(B.Get(Index));
		}
	}
}

TEST(Bitmap, CorrectSize)
{
	pantheon::Bitmap B(16);
	ASSERT_EQ(B.GetSizeBytes(), 16);
	ASSERT_EQ(B.GetSizeBits(), 16 * 8);
}

TEST(PMMAllocator, BasicInit)
{
	pantheon::PhyPageManager Manager(0, 1);
	ASSERT_EQ(Manager.FindFreeAddress()(), 0);
}

TEST(PMMAllocator, ClaimAddresses)
{
	pantheon::PhyPageManager Manager(0, 20);
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		Optional<UINT64> Addr = Manager.FindFreeAddress();
		ASSERT_TRUE(Addr.GetOkay());
		Manager.ClaimAddress(Addr.GetValue());
	}
	Optional<UINT64> Addr = Manager.FindFreeAddress();

	if (Addr.GetOkay())
	{
		ASSERT_EQ(Addr.GetValue(), 16 * 4096);
	}
}

#endif