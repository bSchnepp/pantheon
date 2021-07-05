#include "kern_bitmap.hpp"
#include "kern_bitmap.hpp"

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

pantheon::Bitmap::Bitmap() : pantheon::Bitmap::Bitmap(0)
{
}

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

UINT8 pantheon::Bitmap::GetByte(UINT8 ByteIndex)
{
	if (this->Size <= ByteIndex)
	{
		return 0;
	}
	return this->Area[ByteIndex];
}

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

[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBits() const
{
	return this->Size * 8;
}

[[nodiscard]]
UINT64 pantheon::Bitmap::GetSizeBytes() const
{
	return this->Size;
}