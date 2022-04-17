#ifndef _BOARD_RT_HPP_
#define _BOARD_RT_HPP_

#define PUTCHAR_FUNC(x) WriteSerialChar(x)


void WriteSerialChar(CHAR Char);
void WriteString(const CHAR *String);
void BoardRuntimeInit();

#endif