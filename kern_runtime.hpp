#include <printf.h>
#include <kern_datatypes.hpp>

#ifndef _KERN_RUNTIME_HPP_
#define _KERN_RUNTIME_HPP_

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
T CharStarNumberAtoi(CHAR *Input)
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
class Optional
{
public:
	Optional()
	{
		this->Okay = FALSE;
		this->Value = T();
	}

	Optional(T v)
	{
		this->Okay = TRUE;
		this->Value = v;
	}

	~Optional()
	{

	}

	BOOL operator*()
	{
		return this->Okay;
	}

	T operator()()
	{
		if (this->Okay)
		{
			return this->Value;
		}
		return T();
	}

	BOOL GetOkay()
	{
		return this->Okay;
	}

	T &GetValue()
	{
		return this->Value;
	}

private:
	BOOL Okay;
	T Value;
};

Optional<void*> BasicMalloc(UINT64 Amt);
void BasicFree(void *Addr);

extern "C"
{
	void _putchar(char c);
}

UINT64 StringCompare(void *L, void *R, UINT64 Amt);

#if defined(printf)
#define SERIAL_LOG(fmt, ...) printf((fmt), __VA_ARGS__)
#else
#define SERIAL_LOG(fmt, ...)
#endif

#endif