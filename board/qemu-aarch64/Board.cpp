#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <board/qemu-aarch64/Board.hpp>

/* On qemu-virt, we definitely have a uart. */
#include <Devices/PL011/PL011.hpp>

static constexpr UINT64 DeviceToAddress[] =
{
	[DEVICE_TYPE_UART] = 0x09000000,
};

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

void BoardInit()
{
	pantheon::pl011::PL011Init(DeviceToAddress[DEVICE_TYPE_UART], 0);
}

void _putchar(char c)
{
	if (c == '\n')
	{
		WriteSerialChar('\r');
	}
	WriteSerialChar(c);
}