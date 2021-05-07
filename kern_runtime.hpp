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

extern "C"
{
	void _putchar(char c);
}

#if defined(printf)
#define SERIAL_LOG(fmt, ...) printf((fmt), __VA_ARGS__)
#else
#define SERIAL_LOG(fmt, ...)
#endif

#endif