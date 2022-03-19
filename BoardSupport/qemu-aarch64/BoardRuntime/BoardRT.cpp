#include <BoardDefs.hpp>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

/* On qemu-virt, we definitely have a uart. */
#include <Devices/PL011/PL011.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/gic.hpp>
#include <arch/aarch64/ints.hpp>

#include <arch/aarch64/vmm/vmm.hpp>

void WriteSerialChar(CHAR Char)
{
	pantheon::pl011::PL011WriteChar(0, Char);
}

void WriteString(const CHAR *String)
{
	UINT64 Index = 0;
	while (String[Index])
	{
		CHAR CurChar = String[Index];
		if (CurChar == '\n')
		{
			WriteSerialChar('\r');
		}
		WriteSerialChar(CurChar);

		Index++;
	}
}

void BoardRuntimeInit()
{
	pantheon::pl011::PL011Init(DEVICE_TYPE_UART, 0);
}