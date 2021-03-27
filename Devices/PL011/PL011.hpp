#include <kern_datatypes.hpp>


/* Ported from pious */

namespace pantheon::pl011
{

typedef enum PL011UARTOffset
{
	PL011_DATA_REG = 0x00,
	PL011_RSRECR   = 0x04,
	PL011_FLAG_REG = 0x18,
	PL011_INVALID1 = 0x20,
	PL011_INT_BAUD_REG = 0x24,
	PL011_FRACT_BAUD_REG = 0x28,
	PL011_LINE_CTRL_REG = 0x2C,
	PL011_CTRL_REG = 0x30,
	PL011_INT_FIFO_SEL_REG = 0x34,
	PL011_INT_MASK_CLR_REG = 0x38,
	PL011_INT_STATUS_REG = 0x3C,
	PL011_MASKED_INT_STATUS_REG = 0x40,
	PL011_INT_CLR_REG = 0x44,
	PL011_DMA_CTRL_REG = 0x48,
	PL011_TEST_CTRL_REG = 0x80,
	PL011_ITIP_REG = 0x84,
	PL011_ITOP_REG = 0x88,
	PL011_TDR_REG = 0x8C,
}PL011UARTOffsets;

typedef struct PL011UARTDataReg
{
	union
	{
		UINT32  Raw;
		struct
		{
			UINT16 Invalid;
			UINT8 Invalid2 : 4;
			BOOL OverrunError : 1;
			BOOL BreakError : 1;
			BOOL ParityError : 1;
			BOOL FramingError : 1;
			UINT8 Data;
		};
	};
}__attribute__((__packed__)) UARTDataReg;

typedef struct PL011UARTRSRECR
{
	union
	{
		UINT32  Raw;
		struct
		{
			UINT16 Invalid;
			UINT16 Invalid2 : 12;
			BOOL OverrunError : 1;
			BOOL BreakError : 1;
			BOOL ParityError : 1;
			BOOL FramingError : 1;
		};
	};
}__attribute__((__packed__)) UARTRSRECR;


typedef struct PL011UARTFlagReg
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			UINT8 Invalid2;
			BOOL RI : 1;
			BOOL TXFE : 1;
			BOOL RXFF : 1;
			BOOL TXFF : 1;
			BOOL RXFE : 1;
			BOOL BUSY : 1;
			BOOL DCD : 1;
			BOOL DSR : 1;
			BOOL CTS : 1;
		};
	};
}__attribute__((__packed__)) UARTFlagReg;


typedef struct PL011UARTIntBaudReg
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			UINT16 IntBaud; 
		};
	};
}__attribute__((__packed__)) UARTIntBaudReg;

typedef struct PL011UARTFractBaudReg
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			UINT16 Invalid2 : 11; 
			UINT8 Fraction : 5;
		};
	};
}__attribute__((__packed__)) UARTFractBaudReg;

typedef struct PL011UARTLineControlReg
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			UINT8 Invalid2; 
			BOOL SPS : 1;
			UINT8 WLEN : 2;
			BOOL FEN : 1;
			BOOL STP2 : 1;
			BOOL EPS : 1;
			BOOL PEN : 1;
			BOOL BRK : 1;
		};
	};
}__attribute__((__packed__)) UARTLineControlReg;

typedef struct PL011UARTControlReg
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			BOOL CTSEN : 1;
			BOOL RTSEN : 1;
			BOOL OUT2 : 1;
			BOOL OUT1 : 1;
			BOOL RTS : 1;
			BOOL DTR : 1;
			BOOL RXE : 1;
			BOOL TXE : 1;
			BOOL LBE : 1;
			UINT8 Invalid2 : 4;
			BOOL SIRLP : 1;
			BOOL SIREN : 1;
			BOOL ENABLE : 1;
		};
	};
}__attribute__((__packed__)) UARTControlReg;

typedef struct PL011UARTInterruptFIFOLevelSelect
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT16 Invalid1;
			UINT8 Invalid2 : 4;
			UINT8 RXIFPSEL : 3;
			UINT8 TXIFPSEL : 3;
			UINT8 RXIFLSEL : 3;
			UINT8 TXIFLSEL : 3;
		};
	};
}__attribute__((__packed__)) UARTInterruptFIFOLevelSelect;

typedef struct PL011UARTInterruptMaskSelect
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 21;
			BOOL OEIM : 1;
			BOOL BEIM : 1;
			BOOL PEIM : 1;
			BOOL FEIM : 1;
			BOOL RTIM : 1;
			BOOL TXIM : 1;
			BOOL RXIM : 1;
			BOOL DSRMIM : 1;
			BOOL DCDMIM : 1;
			BOOL CTSMIM : 1;
			BOOL RIMIM : 1;
		};
	};
}__attribute__((__packed__)) UARTInterruptMaskSelect;

typedef struct PL011UARTRawInterrupt
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 21;
			BOOL OERIS : 1;
			BOOL BERIS : 1;
			BOOL PERIS : 1;
			BOOL FERIS : 1;
			BOOL RTRIS : 1;
			BOOL TXRIS : 1;
			BOOL RXRIS : 1;
			BOOL DSRRMIS : 1;
			BOOL DCDRMIS : 1;
			BOOL CTSRMIS : 1;
			BOOL RIRMIS : 1;
		};
	};
}__attribute__((__packed__)) UARTRawInterrupt;

typedef struct PL011UARTMaskedInterrupt
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 21;
			BOOL OEMIS : 1;
			BOOL BEMIS : 1;
			BOOL PEMIS : 1;
			BOOL FEMIS : 1;
			BOOL RTMIS : 1;
			BOOL TXMIS : 1;
			BOOL RXMIS : 1;
			BOOL DSMMMIS : 1;
			BOOL DCDMMIS : 1;
			BOOL CTSMMIS : 1;
			BOOL RIMMIS : 1;
		};
	};
}__attribute__((__packed__)) UARTMaskedInterrupt;

typedef struct PL011UARTInterruptClear
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 21;
			BOOL OEIC : 1;
			BOOL BEIC : 1;
			BOOL PEIC : 1;
			BOOL FEIC : 1;
			BOOL RTIC : 1;
			BOOL TXIC : 1;
			BOOL RXIC : 1;
			BOOL DSRMIC : 1;
			BOOL DCDMIC : 1;
			BOOL CTSMIC : 1;
			BOOL RIMIC : 1;
		};
	};
}__attribute__((__packed__)) UARTInterruptClear;

typedef struct PL011UARTDMAControl
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 29;
			BOOL DMAONERR : 1;
			BOOL TXDMAE : 1;
			BOOL RXDMAE : 1;
		};
	};
}__attribute__((__packed__)) UARTDMAControl;

typedef struct PL011UARTTestControl
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 30;
			BOOL ITCR1 : 1;
			BOOL ITCR0 : 1;
		};
	};
}__attribute__((__packed__)) UARTTestControl;

typedef struct PL011UARTITIP
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 30;
			BOOL ITIP3 : 1;
			UINT8 Invalid2 : 2;
			BOOL ITIP0 : 1;
		};
	};
}__attribute__((__packed__)) UARTITIP;

typedef struct PL011UARTITOP
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 24;
			BOOL ITOP11 : 1;
			BOOL ITOP10 : 1;
			BOOL ITOP9 : 1;
			BOOL ITOP8 : 1;
			BOOL ITOP7 : 1;
			BOOL ITOP6 : 1;
			BOOL ITOP5 : 1;
			BOOL ITOP4 : 1;
			BOOL ITOP3 : 1;
			UINT8 Invalid2 : 2;
			BOOL ITOP0 : 1;
		};
	};
}__attribute__((__packed__)) UARTITOP;

typedef struct PL011UARTDR
{
	union
	{
		UINT32 Raw;
		struct
		{
			UINT32 Invalid1 : 21;
			UINT16 TDR100 : 11;
		};
	};
}__attribute__((__packed__)) UARTTDR;



VOID PL011Init(UINT64 Addr, UINT8 Channel);
VOID PL011Write(UINT8 Channel, PL011UARTOffset Offset, UINT32 Value);
UINT32 PL011Read(UINT8 Channel, PL011UARTOffset Offset);

VOID PL011WriteChar(UINT8 Channel, CHAR Char);

}