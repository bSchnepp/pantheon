#include <stdint.h>

#include "kern_macro.hpp"

#ifndef _INTEGERS_HPP_
#define _INTEGERS_HPP_

uint8_t SwapBytes(uint8_t Item);

uint16_t SwapBytes(uint16_t Item);

uint32_t SwapBytes(uint32_t Item);

uint64_t SwapBytes(uint64_t Item);

int8_t SwapBytes(int8_t Item);

int16_t SwapBytes(int16_t Item);

int32_t SwapBytes(int32_t Item);

int64_t SwapBytes(int64_t Item);

template<typename T>
class Integer
{
public:
	Integer() = default;
	~Integer() = default;

	Integer(T Item)
	{
		union 
		{
			uint32_t Item;
			char Chars[4];
		}EndianCheck = {0x01020304};

		if (EndianCheck.Chars[0] == 0x01)
		{
			this->BackingNum = SwapBytes(Item);
		}
	}

	T GetNum()
	{
		return BackingNum;
	}

	/* FIXME: the host type might not be little endian! */

	T GetNumLE()
	{
		return BackingNum;
	}

	T GetNumBE()
	{
		return SwapBytes(BackingNum);
	}

	T GetNumHost()
	{
		return GetNum();
	}

	Integer operator+(Integer Other)
	{
		return Integer(this->BackingNum + Other.BackingNum);
	}

	Integer operator-(Integer Other)
	{
		return Integer(this->BackingNum - Other.BackingNum);
	}

	Integer operator*(Integer Other)
	{
		return Integer(this->BackingNum * Other.BackingNum);
	}

	Integer operator/(Integer Other)
	{
		return Integer(this->BackingNum / Other.BackingNum);
	}

	Integer operator%(Integer Other)
	{
		return Integer(this->BackingNum % Other.BackingNum);
	}

	Integer& operator+=(Integer Other)
	{
		this->BackingNum += Other.BackingNum;
		return *this;
	}

	Integer& operator-=(Integer Other)
	{
		this->BackingNum -= Other.BackingNum;
		return *this;
	}

	Integer& operator*=(Integer Other)
	{
		this->BackingNum *= Other.BackingNum;
		return *this;
	}

	Integer& operator/=(Integer Other)
	{
		this->BackingNum /= Other.BackingNum;
		return *this;
	}

	Integer& operator%=(Integer Other)
	{
		this->BackingNum %= Other.BackingNum;
		return *this;
	}

private:
	T BackingNum;
};

template<typename T>
class BigEndianInteger
{
public:
	BigEndianInteger() = default;
	~BigEndianInteger() = default;

	BigEndianInteger(T Item)
	{
		union 
		{
			uint32_t Item;
			char Chars[4];
		}EndianCheck = {0x01020304};

		if (EndianCheck.Chars[0] != 0x01)
		{
			this->BackingNum = SwapBytes(Item);
		}
	}

	T GetNum()
	{
		return BackingNum;
	}

	T GetNumLE()
	{
		return SwapBytes(BackingNum);
	}

	T GetNumBE()
	{
		return BackingNum;
	}

	T GetNumHost()
	{
		return GetNumLE();
	}

	BigEndianInteger operator+(BigEndianInteger Other)
	{
		return BigEndianInteger(this->BackingNum + Other.BackingNum);
	}

	BigEndianInteger operator-(BigEndianInteger Other)
	{
		return BigEndianInteger(this->BackingNum - Other.BackingNum);
	}

	BigEndianInteger operator*(BigEndianInteger Other)
	{
		return BigEndianInteger(this->BackingNum * Other.BackingNum);
	}

	BigEndianInteger operator/(BigEndianInteger Other)
	{
		return BigEndianInteger(this->BackingNum / Other.BackingNum);
	}

	BigEndianInteger operator%(BigEndianInteger Other)
	{
		return BigEndianInteger(this->BackingNum % Other.BackingNum);
	}

	BigEndianInteger& operator+=(BigEndianInteger Other)
	{
		this->BackingNum += Other.BackingNum;
		return *this;
	}

	BigEndianInteger& operator-=(BigEndianInteger Other)
	{
		this->BackingNum -= Other.BackingNum;
		return *this;
	}

	BigEndianInteger& operator*=(BigEndianInteger Other)
	{
		this->BackingNum *= Other.BackingNum;
		return *this;
	}

	BigEndianInteger& operator/=(BigEndianInteger Other)
	{
		this->BackingNum /= Other.BackingNum;
		return *this;
	}

	BigEndianInteger& operator%=(BigEndianInteger Other)
	{
		this->BackingNum %= Other.BackingNum;
		return *this;
	}

private:
	T BackingNum;
};

typedef Integer<uint8_t> IntegerU8;
typedef Integer<uint16_t> IntegerU16;
typedef Integer<uint32_t> IntegerU32;
typedef Integer<uint64_t> IntegerU64;

typedef Integer<int8_t> IntegerS8;
typedef Integer<int16_t> IntegerS16;
typedef Integer<int32_t> IntegerS32;
typedef Integer<int64_t> IntegerS64;


typedef BigEndianInteger<uint8_t> BEIntegerU8;
typedef BigEndianInteger<uint16_t> BEIntegerU16;
typedef BigEndianInteger<uint32_t> BEIntegerU32;
typedef BigEndianInteger<uint64_t> BEIntegerU64;

typedef BigEndianInteger<int8_t> BEIntegerS8;
typedef BigEndianInteger<int16_t> BEIntegerS16;
typedef BigEndianInteger<int32_t> BEIntegerS32;
typedef BigEndianInteger<int64_t> BEIntegerS64;


template <typename T>
bool operator==(BigEndianInteger<T> LHS, BigEndianInteger<T> RHS)
{
	return LHS.BackingNum == RHS.BackingNum;
}

template <typename T>
bool operator==(Integer<T> LHS, Integer<T> RHS)
{
	return LHS.BackingNum == RHS.BackingNum;
}

template <typename T>
bool operator==(Integer<T> LHS, BigEndianInteger<T> RHS)
{
	union 
	{
		uint32_t Item;
		char Chars[4];
	}EndianCheck = {0x01020304};

	/* Assume that !BigEndian is little endian. If we are big endian... */
	if (EndianCheck.Chars[0] == 0x01)
	{
		return SwapBytes(RHS.GetNum()) == LHS.GetNum();
	}
	return LHS.GetNum() == SwapBytes(RHS.GetNum());
}

template <typename T>
bool operator==(BigEndianInteger<T> LHS, Integer<T> RHS)
{
	return RHS == LHS;
}

/* TODO: Maybe add big endian + system endian integers? And other math. */
COMPILER_ASSERT(sizeof(Integer<uint8_t>) == sizeof(uint8_t));
COMPILER_ASSERT(sizeof(Integer<uint16_t>) == sizeof(uint16_t));
COMPILER_ASSERT(sizeof(Integer<uint32_t>) == sizeof(uint32_t));
COMPILER_ASSERT(sizeof(Integer<uint64_t>) == sizeof(uint64_t));

COMPILER_ASSERT(sizeof(Integer<int8_t>) == sizeof(int8_t));
COMPILER_ASSERT(sizeof(Integer<int16_t>) == sizeof(int16_t));
COMPILER_ASSERT(sizeof(Integer<int32_t>) == sizeof(int32_t));
COMPILER_ASSERT(sizeof(Integer<int64_t>) == sizeof(int64_t));

#endif