#include <gtest/gtest.h>

#include <kern_runtime.hpp>
#include <kern_container.hpp>

TEST(BasicMalloc, AllocOkay)
{
	Optional<void*> Alloc = BasicMalloc(1024);
	EXPECT_NE(Alloc.GetOkay(), FALSE);
	EXPECT_NE(Alloc.GetValue(), nullptr);
}

TEST(BasicMalloc, TwoAllocNotEqual)
{
	Optional<void*> Alloc1 = BasicMalloc(4);
	Optional<void*> Alloc2 = BasicMalloc(4);
	EXPECT_EQ(Alloc1.GetOkay(), TRUE);
	EXPECT_EQ(Alloc2.GetOkay(), TRUE);
	EXPECT_EQ(Alloc1.GetOkay(), Alloc2.GetOkay());
	EXPECT_NE(Alloc1(), Alloc2());
}

TEST(BasicMalloc, FreeAddrEq)
{
	Optional<void*> Alloc1 = BasicMalloc(4);
	Optional<void*> Alloc2 = BasicMalloc(4);
	EXPECT_EQ(Alloc1.GetOkay(), TRUE);
	EXPECT_EQ(Alloc2.GetOkay(), TRUE);

	UINT32 *IntVal1 = (UINT32*)(Alloc1());
	UINT32 *IntVal2 = (UINT32*)(Alloc2());

	*IntVal1 = 0x01020304;
	*IntVal2 = 0x05060708;

	EXPECT_EQ(*IntVal1, 0x01020304);
	EXPECT_EQ(*IntVal2, 0x05060708);
	BasicFree(Alloc1.GetValue());

	EXPECT_EQ(*IntVal2, 0x05060708);
}

TEST(BasicMalloc, SingleFree)
{
	Optional<void*> Alloc1 = BasicMalloc(4);
	EXPECT_EQ(Alloc1.GetOkay(), TRUE);
	UINT32 *IntVal1 = (UINT32*)(Alloc1());

	*IntVal1 = 0x01020304;
	EXPECT_EQ(*IntVal1, 0x01020304);
	BasicFree(Alloc1.GetValue());
}

TEST(BasicMalloc, ManyAllocManyFree)
{
	for (int index = 0; index < 32; ++index)
	{
		Optional<void*> Alloc1 = BasicMalloc(4);
		EXPECT_EQ(Alloc1.GetOkay(), TRUE);
		UINT32 *IntVal1 = (UINT32*)(Alloc1());

		*IntVal1 = 0x01020304;
		EXPECT_EQ(*IntVal1, 0x01020304);
		BasicFree(Alloc1.GetValue());
	}
}

TEST(ArrayList, BasicCreate)
{
	ArrayList<INT32> Arr;
	ASSERT_EQ(Arr.Size(), 0);
}

TEST(ArrayList, BasicDelete)
{
	ArrayList<INT32> Arr;
	Arr.Delete(1);
	ASSERT_EQ(Arr.Size(), 0);
}

TEST(ArrayList, BasicContains)
{
	ArrayList<INT32> Arr;
	ASSERT_FALSE(Arr.Contains(4));
}

TEST(ArrayList, BasicAdd)
{
	ArrayList<INT32> Arr;
	Arr.Add(3);
	Arr.Add(4);
	ASSERT_EQ(Arr.Size(), 2);
}

TEST(ArrayList, BasicGet)
{
	ArrayList<INT32> Arr;
	Arr.Add(3);
	Arr.Add(4);
	ASSERT_EQ(Arr.Get(0), 3);
	ASSERT_EQ(Arr.Get(1), 4);
}

TEST(ArrayList, ReallocAdd)
{
	ArrayList<INT32> Arr;
	Arr.Add(3);
	Arr.Add(4);
	Arr.Add(5);
	Arr.Add(6);
	ASSERT_EQ(Arr.Size(), 4);
}

TEST(ArrayList, ReallocIndex)
{
	ArrayList<INT32> Arr;
	Arr.Add(3);
	Arr.Add(4);
	Arr.Add(5);
	Arr.Add(6);
	ASSERT_TRUE(Arr[3].GetOkay());
	ASSERT_EQ(Arr[3].GetValue(), 6);
}

TEST(ArrayList, ReallocIndexNotExisting)
{
	ArrayList<INT32> Arr;
	Arr.Add(3);
	Arr.Add(4);
	Arr.Add(5);
	Arr.Add(6);
	ASSERT_FALSE(Arr[100].GetOkay());
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}