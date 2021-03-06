#include <kern_datatypes.hpp>
#include "kern_rawbitmap.hpp"

#ifndef _KERN_BITMAP_
#define _KERN_BITMAP_

namespace pantheon
{

class Bitmap
{
public:
	Bitmap();
	Bitmap(UINT64 ByteAmount);
	~Bitmap();

	BOOL Get(UINT64 Index);
	VOID Set(UINT64 Index, BOOL Bit);

	UINT8 GetByte(UINT64 ByteIndex);

	[[nodiscard]] UINT64 GetSizeBits() const;
	[[nodiscard]] UINT64 GetSizeBytes() const;

private:
	pantheon::RawBitmap RawArea;
};

}

#endif