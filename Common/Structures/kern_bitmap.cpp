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
		return;
	}
	this->RawArea = pantheon::RawBitmap((UINT8*)MaybeMem.GetValue(), ByteAmt);
}

pantheon::Bitmap::~Bitmap()
{
	UINT8 *Area = this->RawArea.GetAddress();
	if (Area)
	{
		BasicFree(Area);
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
	return this->RawArea.Get(Index);
}

/**
 * \~english @brief Obtains the value of an entire byte in the bitmap
 * \~english @param[in] Index The byte to index, with 0 being the byte containing bits [0:7]
 * \~english @return The byte, expressed as an unsigned 8-bit integer at that location or 0 if not present
 * \~english @author Brian Schnepp
 */
UINT8 pantheon::Bitmap::GetByte(UINT64 ByteIndex)
{
	return this->RawArea.GetByte(ByteIndex);
}

/**
 * \~english @brief Sets a particular value in the bitmap
 * \~english @param[in] Index The bit to set, with 0 being the first bit
 * \~english @param[in] Bit The value to set the bit to, with FALSE as 0 and TRUE as 1
 * \~english @author Brian Schnepp
 */
VOID pantheon::Bitmap::Set(UINT64 Index, BOOL Bit)
{
	this->RawArea.Set(Index, Bit);
}

/**
 * \~english @brief Gets the number of bits in the bitmap
 * \~english @return The number of bits which are valid in this bitmap
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBits() const
{
	return this->RawArea.GetSizeBits();
}

/**
 * \~english @brief Gets the number of bytes in the bitmap
 * \~english @return The number of bytes which are valid in this bitmap
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBytes() const
{
	return this->RawArea.GetSizeBytes();
}