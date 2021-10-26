#include <iostream>

#include <arch.hpp>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

void WriteSerialChar(CHAR Char)
{
}

void WriteString(const CHAR *String)
{

}

void BoardInit(pantheon::vmm::PageAllocator &PageAllocator)
{
	PANTHEON_UNUSED(PageAllocator);
}