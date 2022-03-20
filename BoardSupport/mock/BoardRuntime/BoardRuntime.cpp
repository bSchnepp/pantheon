#include <iostream>

#include <kern_datatypes.hpp>
#include <BoardRuntime/BoardRT.hpp>

void WriteSerialChar(CHAR Char)
{
	std::cout << Char;
}

void WriteString(const CHAR *String)
{
	std::cout << String << std::endl;
}

void BoardRuntimeInit()
{

}