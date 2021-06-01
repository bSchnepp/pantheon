#include <gtest/gtest.h>

#include <kern_runtime.hpp>
#include <kern_container.hpp>

#ifndef COMMON_TESTS_HPP_
#define COMMON_TESTS_HPP_

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
	ASSERT_EQ(Arr.Get(0), Arr[0].GetValue());
	ASSERT_EQ(Arr.Get(1), Arr[1].GetValue());
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

TEST(Atoi, U8AtoiBase10)
{
	UINT8 U8 = CharStarNumberAtoi<UINT8>("8");
	ASSERT_EQ(U8, 8);
}

TEST(Atoi, U16AtoiBase10)
{
	UINT16 U16 = CharStarNumberAtoi<UINT16>("8");
	ASSERT_EQ(U16, 8);
}

TEST(Atoi, U32AtoiBase10)
{
	UINT32 U32 = CharStarNumberAtoi<UINT32>("8");
	ASSERT_EQ(U32, 8);
}

TEST(Atoi, U64AtoiBase10)
{
	UINT64 U64 = CharStarNumberAtoi<UINT64>("8");
	ASSERT_EQ(U64, 8);
}

TEST(Atoi, BadU64AtoiBase10)
{
	UINT64 U64 = CharStarNumberAtoi<UINT64>("eight");
	ASSERT_NE(U64, 8);
}

TEST(Atoi, U8AtoiBase16)
{
	UINT8 U8 = CharStarNumberAtoiB16<UINT8>("8");
	ASSERT_EQ(U8, 8);
}

TEST(Atoi, U16AtoiBase16)
{
	UINT16 U16 = CharStarNumberAtoiB16<UINT16>("8");
	ASSERT_EQ(U16, 8);
}

TEST(Atoi, U32AtoiBase16)
{
	UINT32 U32 = CharStarNumberAtoiB16<UINT32>("8");
	ASSERT_EQ(U32, 8);
}

TEST(Atoi, U64AtoiBase16)
{
	UINT64 U64 = CharStarNumberAtoiB16<UINT64>("8");
	ASSERT_EQ(U64, 8);
}

TEST(Atoi, BadU64AtoiBase16)
{
	UINT64 U64 = CharStarNumberAtoiB16<UINT64>("eight");
	ASSERT_NE(U64, 8);
}

TEST(Atoi, U8AtoiBase16Hex)
{
	UINT8 U8 = CharStarNumberAtoiB16<UINT8>("f");
	ASSERT_EQ(U8, 15);
}

TEST(Atoi, U16AtoiBase16Hex)
{
	UINT16 U16 = CharStarNumberAtoiB16<UINT16>("f");
	ASSERT_EQ(U16, 15);
}

TEST(Atoi, U32AtoiBase16Hex)
{
	UINT32 U32 = CharStarNumberAtoiB16<UINT32>("f");
	ASSERT_EQ(U32, 15);
}

TEST(Atoi, U64AtoiBase16Hex)
{
	UINT64 U64 = CharStarNumberAtoiB16<UINT64>("f");
	ASSERT_EQ(U64, 15);
}

TEST(Atoi, U8AtoiBase16HexUpper)
{
	UINT8 U8 = CharStarNumberAtoiB16<UINT8>("F");
	ASSERT_EQ(U8, 15);
}

TEST(Atoi, U16AtoiBase16HexUpper)
{
	UINT16 U16 = CharStarNumberAtoiB16<UINT16>("F");
	ASSERT_EQ(U16, 15);
}

TEST(Atoi, U32AtoiBase16HexUpper)
{
	UINT32 U32 = CharStarNumberAtoiB16<UINT32>("F");
	ASSERT_EQ(U32, 15);
}

TEST(Atoi, U64AtoiBase16HexUpper)
{
	UINT64 U64 = CharStarNumberAtoiB16<UINT64>("F");
	ASSERT_EQ(U64, 15);
}

TEST(Atoi, BadU64AtoiBase16Hex)
{
	UINT64 U64 = CharStarNumberAtoiB16<UINT64>("fifteen");
	ASSERT_NE(U64, 15);
}

TEST(StringCompare, GoodStringCompare)
{
	const CHAR *One = "one";
	const char *Two = "one";
	ASSERT_TRUE(StringCompare(One, Two, 3));
}

TEST(StringCompare, GoodStringCompareEndOfString)
{
	const CHAR *One = "one";
	const char *Two = "one";
	ASSERT_TRUE(StringCompare(One, Two, 300));
}

TEST(StringCompare, BadStringCompare)
{
	const CHAR *One = "one";
	const char *Two = "true";
	ASSERT_FALSE(StringCompare(One, Two, 3));
}

TEST(StringCompare, GoodSubStringCompare)
{
	const CHAR *One = "o";
	const char *Two = "on";
	ASSERT_TRUE(StringCompare(One, Two, 1));
}

TEST(StringCompare, NotEqualGoodSubStringCompare)
{
	const CHAR *One = "one";
	const char *Two = "oneplusone";
	ASSERT_TRUE(StringCompare(One, Two, 3));
}

TEST(StringCompare, BadSubStringCompare)
{
	const CHAR *One = "o";
	const char *Two = "on";
	ASSERT_FALSE(StringCompare(One, Two, 2));
}

TEST(StrLen, LenOne)
{
	const CHAR *One = "o";
	ASSERT_EQ(ConstStrLen(One), 1);
}

TEST(StrLen, LenMany)
{
	const CHAR *One = "one";
	ASSERT_EQ(ConstStrLen(One), 3);
}

TEST(StrLen, LenZero)
{
	const CHAR *One = "";
	ASSERT_EQ(ConstStrLen(One), 0);
}

TEST(ClearBuffer, Clear)
{
	char SomeMem[1024];
	ClearBuffer(SomeMem, 1024);
	for (char &Item : SomeMem)
	{
		ASSERT_EQ(Item, 0);
	}
}

TEST(VolatileMMIO, U8)
{
	UINT8 Item;
	WriteMMIOU8((UINT64)(&Item), 4);
	ASSERT_EQ(ReadMMIOU8((UINT64)(&Item)), 4);
}

TEST(VolatileMMIO, U16)
{
	UINT16 Item;
	WriteMMIOU16((UINT64)(&Item), 4);
	ASSERT_EQ(ReadMMIOU16((UINT64)(&Item)), 4);
}

TEST(VolatileMMIO, U32)
{
	UINT32 Item;
	WriteMMIOU32((UINT64)(&Item), 4);
	ASSERT_EQ(ReadMMIOU32((UINT64)(&Item)), 4);
}

TEST(VolatileMMIO, U64)
{
	UINT64 Item;
	WriteMMIOU64((UINT64)(&Item), 4);
	ASSERT_EQ(ReadMMIOU64((UINT64)(&Item)), 4);
}

TEST(VolatileMMIO, S8)
{
	INT8 Item;
	WriteMMIOS8((UINT64)(&Item), -4);
	ASSERT_EQ(ReadMMIOS8((UINT64)(&Item)), -4);
}

TEST(VolatileMMIO, S16)
{
	INT16 Item;
	WriteMMIOS16((UINT64)(&Item), -4);
	ASSERT_EQ(ReadMMIOS16((UINT64)(&Item)), -4);
}

TEST(VolatileMMIO, S32)
{
	INT32 Item;
	WriteMMIOS32((UINT64)(&Item), -4);
	ASSERT_EQ(ReadMMIOS32((UINT64)(&Item)), -4);
}

TEST(VolatileMMIO, S64)
{
	INT64 Item;
	WriteMMIOS64((UINT64)(&Item), -4);
	ASSERT_EQ(ReadMMIOS64((UINT64)(&Item)), -4);
}

#endif