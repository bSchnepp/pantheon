#include "kern_bitmap.hpp"
#include "kern_bitmap.hpp"

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

/**
 * \~english @brief Creates an empty bitmap
 * \~english @author Brian Schnepp
 */
pantheon::Bitmap::Bitmap() : pantheon::Bitmap::Bitmap(0)
{
}

/**
 * \~english @brief Creates a bitmap with a given number of bytes to use for storage
 * \~english @param[in] ByteAmt The amount of bytes to allocate
 * \~english @author Brian Schnepp
 */
pantheon::Bitmap::Bitmap(UINT64 ByteAmt)
{
	Optional<void*> MaybeMem = BasicMalloc(ByteAmt);
	if (!MaybeMem.GetOkay())
	{
		this->Area = nullptr;
		this->Size = 0;
		return;
	}
	this->Size = ByteAmt;
	this->Area = (UINT8*)(MaybeMem.GetValue());
	ClearBuffer((CHAR*)this->Area, ByteAmt);
}

pantheon::Bitmap::~Bitmap()
{
	if (this->Area)
	{
		BasicFree(this->Area);
	}
}

/**
 * \~english @brief Obtains the value of a bit in the bitmap
 * \~english @param[in] Index The bit to index, with 0 being the first bit
 * \~english @return The status of the bit: TRUE for 1, FALSE for 0 or not present
 * \~english @author Brian Schnepp
 */
BOOL pantheon::Bitmap::Get(UINT64 Index)
{
	UINT64 Byte = Index / 8;
	UINT8 Offset = Index % 8;
	if (this->Size <= Byte)
	{
		return FALSE;
	}

	UINT8 ByteEntry = this->Area[Byte];
	return (ByteEntry & (1 << Offset)) != 0;
}

/**
 * \~english @brief Obtains the value of an entire byte in the bitmap
 * \~english @param[in] Index The byte to index, with 0 being the byte containing bits [0:7]
 * \~english @return The byte, expressed as an unsigned 8-bit integer at that location or 0 if not present
 * \~english @author Brian Schnepp
 */
UINT8 pantheon::Bitmap::GetByte(UINT64 ByteIndex)
{
	if (this->Size <= ByteIndex)
	{
		return 0;
	}
	return this->Area[ByteIndex];
}

/**
 * \~english @brief Sets a particular value in the bitmap
 * \~english @param[in] Index The bit to set, with 0 being the first bit
 * \~english @param[in] Bit The value to set the bit to, with FALSE as 0 and TRUE as 1
 * \~english @author Brian Schnepp
 */
VOID pantheon::Bitmap::Set(UINT64 Index, BOOL Bit)
{
	UINT64 Byte = Index / 8;
	UINT8 Offset = Index % 8;
	if (this->Size <= Byte)
	{
		return;
	}

	UINT8 ByteEntry = this->Area[Byte];
	UINT8 Bitmask = (1 << Offset);
	ByteEntry &= ~Bitmask;
	ByteEntry |= (Bit << Offset);
	this->Area[Byte] = ByteEntry;
}

/**
 * \~english @brief Gets the number of bits in the bitmap
 * \~english @return The number of bits which are valid in this bitmap
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBits() const
{
	return this->Size * 8;
}

/**
 * \~english @brief Gets the number of bytes in the bitmap
 * \~english @return The number of bytes which are valid in this bitmap
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBytes() const
{
	return this->Size;
}