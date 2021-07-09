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

TEST(Bitmap, BasicFillThenUnset)
{
	pantheon::Bitmap B(16);
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		B.Set(Index, TRUE);
	}
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		B.Set(Index, FALSE);
	}
	for (UINT64 Index = 0; Index < 16; ++Index)
	{
		ASSERT_FALSE(B.Get(Index));
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

TEST(PMMAllocator, PhysManager)
{
	pantheon::GlobalPhyPageManager GlobalManager;
	GlobalManager.AddArea(0, 1024);
	GlobalManager.AddArea(0x2000000, 24);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 0);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 4096);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 8192);
}

TEST(PMMAllocator, PhysManagerIrregularArea)
{
	pantheon::GlobalPhyPageManager GlobalManager;
	GlobalManager.AddArea(0, 1024);
	GlobalManager.AddArea(0x2000000, 26);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 0);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 4096);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 8192);
}

TEST(PMMAllocator, PhysManagerNoFree)
{
	pantheon::GlobalPhyPageManager GlobalManager;
	ASSERT_FALSE(GlobalManager.FindAndClaimFirstFreeAddress().GetOkay());
}

TEST(PMMAllocator, PhysManagerIrregularAreaClaim)
{
	pantheon::GlobalPhyPageManager GlobalManager;
	GlobalManager.AddArea(0, 1024);
	GlobalManager.AddArea(0x2000000, 26);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 0);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 4096);
	ASSERT_EQ(GlobalManager.FindAndClaimFirstFreeAddress()(), 8192);
	ASSERT_TRUE(GlobalManager.CheckClaimed(0));
	ASSERT_TRUE(GlobalManager.CheckClaimed(4096));
	GlobalManager.FreeAddress(4096);
	GlobalManager.FreeAddress(16384);
	ASSERT_FALSE(GlobalManager.CheckClaimed(4096));
	ASSERT_EQ(GlobalManager.FindFreeAddress()(), 4096);
}

TEST(PMMAllocator, PhysManagerClaimRaw)
{
	pantheon::GlobalPhyPageManager GlobalManager;
	GlobalManager.AddArea(0, 1024);
	GlobalManager.ClaimAddress(4096);
	ASSERT_TRUE(GlobalManager.CheckClaimed(4096));
	GlobalManager.FreeAddress(4096);
	GlobalManager.FreeAddress(16384);
	ASSERT_FALSE(GlobalManager.CheckClaimed(4096));
}

#endif