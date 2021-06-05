#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <board/qemu-aarch64/Board.hpp>

/* On qemu-virt, we definitely have a uart. */
#include <Devices/PL011/PL011.hpp>

#include <arch/aarch64/gic.hpp>
#include <arch/aarch64/ints.hpp>

typedef enum DeviceToAddress
{
	DEVICE_TYPE_UART = 0x09000000,
	DEVICE_TYPE_GIC_DIST = 0x08000000,
	DEVICE_TYPE_GIC_CPU = 0x08010000,
}DeviceToAddress;


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

extern char interrupt_table[];

void BoardInit()
{
	pantheon::pl011::PL011Init(DEVICE_TYPE_UART, 0);

	pantheon::arm::GICSetMMIOAddr(pantheon::arm::GIC_CLASS_DISTRIBUTOR, DEVICE_TYPE_GIC_DIST);
	pantheon::arm::GICSetMMIOAddr(pantheon::arm::GIC_CLASS_CPU_INTERFACE, DEVICE_TYPE_GIC_CPU);

	pantheon::arm::GICInit();
}

VOID PerCoreBoardInit()
{
	pantheon::arm::LoadInterruptTable(&interrupt_table);

	/* HACK: this only works on qemu aarch64 virt... */
	const static UINT32 TimerIRQ = 30;
	pantheon::arm::GICSetConfig(TimerIRQ, 2);
	pantheon::arm::GICIgnoreInterrupt(TimerIRQ);
	pantheon::arm::GICEnableInterrupt(TimerIRQ);
}

void _putchar(char c)
{
	if (c == '\n')
	{
		WriteSerialChar('\r');
	}
	WriteSerialChar(c);
}