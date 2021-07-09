#include <kern_datatypes.hpp>

#ifndef _KERN_RAWBITMAP_HPP_
#define _KERN_RAWBITMAP_HPP_

namespace pantheon
{

class RawBitmap
{
public:
	RawBitmap();
	RawBitmap(UINT8 *Area, UINT64 ByteAmount);
	~RawBitmap();

	BOOL Get(UINT64 Index);
	VOID Set(UINT64 Index, BOOL Bit);

	UINT8 GetByte(UINT64 ByteIndex);

	[[nodiscard]] UINT64 GetSizeBits() const;
	[[nodiscard]] UINT64 GetSizeBytes() const;

	[[nodiscard]] UINT8 *GetAddress() const;

private:
	UINT64 Size;
	UINT8 *Area;
};

}

#endif