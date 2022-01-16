#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <BoardSupport/qemu-aarch64/Board.hpp>

/* On qemu-virt, we definitely have a uart. */
#include <Devices/PL011/PL011.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/gic.hpp>
#include <arch/aarch64/ints.hpp>

#include <arch/aarch64/vmm/vmm.hpp>

typedef enum DeviceToAddress
{
	DEVICE_TYPE_UART = 0xFFFFFFF7FFDF0000,
	DEVICE_TYPE_GIC_DIST = 0xFFFFFFF7FFDF2000,
	DEVICE_TYPE_GIC_CPU = 0xFFFFFFF7FFDF4000,
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

extern "C" void BoardInit(pantheon::vmm::PageTable *TTBR1, pantheon::vmm::PageAllocator &PageAllocator)
{
	/* FIXME: Handle paging for all the devices as needed. */
	pantheon::vmm::PageTableEntry DeviceMMIOEntry;

	DeviceMMIOEntry.SetBlock(TRUE);
	DeviceMMIOEntry.SetMapped(TRUE);
	DeviceMMIOEntry.SetUserNoExecute(TRUE);
	DeviceMMIOEntry.SetKernelNoExecute(FALSE);
	DeviceMMIOEntry.SetUserAccessible(TRUE);
	DeviceMMIOEntry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_NONE);
	DeviceMMIOEntry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	DeviceMMIOEntry.SetPagePermissions(pantheon::vmm::PAGE_PERMISSION_KERNEL_RW);
	DeviceMMIOEntry.SetMAIREntry(pantheon::vmm::MAIREntry_0);

	/* This seems odd, but must use MapLower since higher half isn't executing yet. */
	PageAllocator.MapLower(TTBR1, DEVICE_TYPE_UART, 0x09000000, pantheon::vmm::BlockSize::L3BlockSize, DeviceMMIOEntry);
	PageAllocator.MapLower(TTBR1, DEVICE_TYPE_GIC_DIST, 0x08000000, pantheon::vmm::BlockSize::L3BlockSize, DeviceMMIOEntry);
	PageAllocator.MapLower(TTBR1, DEVICE_TYPE_GIC_CPU, 0x08010000, pantheon::vmm::BlockSize::L3BlockSize, DeviceMMIOEntry);

	pantheon::pl011::PL011Init(DEVICE_TYPE_UART, 0);

	pantheon::arm::GICSetMMIOAddr(pantheon::arm::GIC_CLASS_DISTRIBUTOR, DEVICE_TYPE_GIC_DIST);
	pantheon::arm::GICSetMMIOAddr(pantheon::arm::GIC_CLASS_CPU_INTERFACE, DEVICE_TYPE_GIC_CPU);
	pantheon::arm::GICInit();
}

VOID PerCoreBoardInit()
{
	/* HACK: this only works on qemu aarch64 virt... */
	const static UINT32 TimerIRQ = 30;
	pantheon::arm::GICSetConfig(TimerIRQ, 2);
	pantheon::arm::GICIgnoreInterrupt(TimerIRQ);
	pantheon::arm::GICEnableInterrupt(TimerIRQ);
	pantheon::arm::GICInitCore();
}

void _putchar(char c)
{
	if (c == '\n')
	{
		WriteSerialChar('\r');
	}
	WriteSerialChar(c);
}