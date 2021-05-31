#include <gtest/gtest.h>

#include <kern_runtime.hpp>

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

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}