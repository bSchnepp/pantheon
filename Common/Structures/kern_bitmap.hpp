#include <kern_datatypes.hpp>

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

	UINT8 GetByte(UINT8 ByteIndex);

	[[nodiscard]] UINT64 GetSizeBits() const;
	[[nodiscard]] UINT64 GetSizeBytes() const;

private:
	UINT64 Size;
	UINT8 *Area;
};

}

#endif