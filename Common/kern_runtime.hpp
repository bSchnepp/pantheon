#include <Common/kern_datatypes.hpp>
#include <Structures/kern_optional.hpp>

#ifndef _KERN_RUNTIME_HPP_
#define _KERN_RUNTIME_HPP_

#define FORCE_INLINE inline __attribute__((always_inline))

VOID WriteMMIOU64(UINT64 Addr, UINT64 Value);
VOID WriteMMIOU32(UINT64 Addr, UINT32 Value);
VOID WriteMMIOU16(UINT64 Addr, UINT16 Value);
VOID WriteMMIOU8(UINT64 Addr, UINT8 Value);
VOID WriteMMIOS64(UINT64 Addr, INT64 Value);
VOID WriteMMIOS32(UINT64 Addr, INT32 Value);
VOID WriteMMIOS16(UINT64 Addr, INT16 Value);
VOID WriteMMIOS8(UINT64 Addr, INT8 Value);
UINT64 ReadMMIOU64(UINT64 Addr);
UINT32 ReadMMIOU32(UINT64 Addr);
UINT16 ReadMMIOU16(UINT64 Addr);
UINT8  ReadMMIOU8(UINT64 Addr);
INT64 ReadMMIOS64(UINT64 Addr);
INT32 ReadMMIOS32(UINT64 Addr);
INT16 ReadMMIOS16(UINT64 Addr);
INT8  ReadMMIOS8(UINT64 Addr);

void BoardInit();
void WriteSerialChar(CHAR Char);
void WriteString(const CHAR *String);


template <typename T>
T FORCE_INLINE CharStarNumberAtoi(const CHAR *Input)
{
	T Result = 0;
	UINT64 Index = 0;
	for (Index = 0; Input[Index] != '\0'; ++Index)
	{
		/* Move the place over by 10. */
		Result *= 10;

		/* Find the diff between the current item and the ASCII code 
		 * for 0. Since they're all ordered nicely, there's no mapping
		 * function needed--each number is direct mapped to it's value
		 * plus offset.
		 */
		Result += (Input[Index] - '0');
	}
	return Result;
}

template <typename T>
T FORCE_INLINE CharStarNumberAtoiB16(const CHAR *Input)
{
	T Result = 0;
	UINT64 Index = 0;
	for (Index = 0; Input[Index] != '\0'; ++Index)
	{
		/* Move the place over by 16. */
		Result *= 16;

		/* Find the diff between the current item and the ASCII code 
		 * for 0. Since they're all ordered nicely, there's no mapping
		 * function needed--each number is direct mapped to it's value
		 * plus offset.
		 */
		if (Input[Index] == 'a' || Input[Index] == 'A')
		{
			Result += 10;
		}
		else if (Input[Index] == 'b' || Input[Index] == 'B')
		{
			Result += 11;
		}
		else if (Input[Index] == 'c' || Input[Index] == 'C')
		{
			Result += 12;
		}
		else if (Input[Index] == 'd' || Input[Index] == 'D')
		{
			Result += 13;
		}
		else if (Input[Index] == 'e' || Input[Index] == 'E')
		{
			Result += 14;
		}
		else if (Input[Index] == 'f' || Input[Index] == 'F')
		{
			Result += 15;
		}
		else
		{
			Result += (Input[Index] - '0');
		}
	}
	return Result;
}


template<typename T>
T FORCE_INLINE Max(T L, T R)
{
	return (L > R) ? L : R;
}

template<typename T>
T FORCE_INLINE Min(T L, T R)
{
	return (L > R) ? R : L;
}

template<typename T>
constexpr FORCE_INLINE T Align(T Amt, T Align)
{
	return ~(Align - 1) & (((Amt) + (Align - 1)));
}

template<typename T>
constexpr FORCE_INLINE BOOL IsAligned(T Val, T AlignVal)
{
	return Align(Val, AlignVal) == Val;
}

Optional<void*> BasicMalloc(UINT64 Amt);
void BasicFree(void *Addr);

extern "C"
{
	void _putchar(char c);
}

BOOL FORCE_INLINE StringCompare(const CHAR *Arg1, const CHAR *Arg2, UINT64 Amt)
{
	for (UINT64 Index = 0; Index < Amt; ++Index)
	{
		if (Arg1[Index] != Arg2[Index])
		{
			return FALSE;
		}

		if (Arg1[Index] == 0)
		{
			break;
		}
	}
	return TRUE;
}

void FORCE_INLINE SetBufferBytes(CHAR *Location, UINT8 Value, UINT32 Amount)
{
	UINT32 Index = 0;

	UINT64 *AsUINT64 = (UINT64*)Location;
	for (Index = 0; Index < Amount; Index += 8)
	{
		AsUINT64[Index / 8] = 0;
	}

	for (; Index < Amount; ++Index)
	{
		Location[Index] = Value;
	}
}

void FORCE_INLINE ClearBuffer(CHAR *Location, UINT32 Amount)
{
	SetBufferBytes(Location, 0x00, Amount);
}

void FORCE_INLINE CopyMemory(VOID *Dest, VOID *Src, UINT64 Amt)
{
	UINT64 Index = 0;
	UINT64 RawDest = (UINT64)Dest;
	UINT64 RawSrc = (UINT64)Src;

	CHAR *DestAsChar = reinterpret_cast<CHAR*>(RawDest);
	CHAR *SrcAsChar = reinterpret_cast<CHAR*>(RawSrc);

	typedef UINT64 __attribute__((__may_alias__)) AUINT64;
	if (((RawDest & 0x07) == 0) && ((RawSrc & 0x07) == 0))
	{
		AUINT64 *DestAsU64 = reinterpret_cast<AUINT64*>(RawDest);
		AUINT64 *SrcAs64 = reinterpret_cast<AUINT64*>(RawSrc);
		for (; (Index * sizeof(UINT64)) < Amt; Index++)
		{
			DestAsU64[Index] = SrcAs64[Index];
		} 
	}

	Index *= sizeof(UINT64);


	for (; Index < Amt; ++Index)
	{
		DestAsChar[Index] = SrcAsChar[Index];
	}
}

constexpr UINT32 ConstStrLen(const CHAR *Str)
{
	UINT32 Count = 0;
	for (Count = 0; Str[Count] != '\0'; ++Count)
	{
	}
	return Count;
}


void SERIAL_LOG(const char *Fmt, ...);
void SERIAL_LOG_UNSAFE(const char *Fmt, ...);

namespace pantheon
{

void InitBasicRuntime();
void InitBasicMemory();
[[noreturn]] void StopError(const char *Reason, void *Source = nullptr);
[[noreturn]] void StopErrorFmt(const char *Reason, ...);
BOOL Panicked();

}

#define OBJECT_SELF_ASSERT(x) if ((x) == nullptr) { StopError("called method was nullptr"); }

#endif