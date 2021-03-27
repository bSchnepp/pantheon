#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <Devices/PL011/PL011.hpp>

/* Support up to 8 built in UARTs. */
static UINT64 Channels[8];

VOID pantheon::pl011::PL011Init(UINT64 Addr, UINT8 Channel)
{
	UINT8 RealChannel = Channel % 8;
	Channels[RealChannel] = Addr;

	/* First, disable everything. */
	PL011Write(RealChannel, PL011_CTRL_REG, 0);

	pantheon::pl011::PL011UARTInterruptClear Ints;
	pantheon::pl011::PL011UARTIntBaudReg IntBaud;
	pantheon::pl011::PL011UARTFractBaudReg FractBaud;
	pantheon::pl011::PL011UARTLineControlReg LineCtrl;
	pantheon::pl011::PL011UARTControlReg Control;

	Ints.Raw  = 0;
	IntBaud.Raw = 0;
	FractBaud.Raw = 0;
	LineCtrl.Raw = 0;
	Control.Raw = 0;

	Ints.Raw  = 0x7FF;
	IntBaud.IntBaud = 0x02;
	FractBaud.Fraction = 0b1011;
	LineCtrl.SPS = FALSE;
	LineCtrl.WLEN = 0b11;
	Control.TXE  = TRUE;
	Control.RXE = TRUE;
	Control.ENABLE = TRUE;

	pantheon::pl011::PL011Write(RealChannel, PL011_INT_CLR_REG, Ints.Raw);
	pantheon::pl011::PL011Write(RealChannel, PL011_INT_BAUD_REG, IntBaud.Raw);
	pantheon::pl011::PL011Write(RealChannel, PL011_FRACT_BAUD_REG, FractBaud.Raw);
	pantheon::pl011::PL011Write(RealChannel, PL011_LINE_CTRL_REG, LineCtrl.Raw);
	//pantheon::pl011::PL011Write(RealChannel, PL011_CTRL_REG, Control.Raw);
}

VOID pantheon::pl011::PL011Write(UINT8 Channel, PL011UARTOffset Offset, UINT32 Value)
{
	UINT64 Addr = Channels[Channel] + ((UINT64)(Offset));
	WriteMMIOU32(Addr, Value);
}

UINT32 pantheon::pl011::PL011Read(UINT8 Channel, PL011UARTOffset Offset)
{
	UINT64 Addr = Channels[Channel] + ((UINT64)(Offset));
	return ReadMMIOU32(Addr);
}

VOID pantheon::pl011::PL011WriteChar(UINT8 Channel, CHAR Char)
{
	UINT8 RealChannel = Channel % 8;
	UINT64 UartAddr = Channels[RealChannel];
	UINT64 UartData = UartAddr + pantheon::pl011::PL011_DATA_REG;

	UINT32 Unavailable = PL011Read(RealChannel, pantheon::pl011::PL011_FLAG_REG) & 0x20;
	while (Unavailable)
	{
		Unavailable = PL011Read(RealChannel, pantheon::pl011::PL011_FLAG_REG) & 0x20;
	}
	PL011Write(RealChannel, pantheon::pl011::PL011_DATA_REG, Char);
}