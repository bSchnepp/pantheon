#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <BoardDefs.hpp>
#include <BoardSupport/qemu-aarch64/BoardInit/Board.hpp>

#include <arch/aarch64/arch.hpp>
#include <arch/aarch64/gic.hpp>
#include <arch/aarch64/ints.hpp>

#include <arch/aarch64/vmm/vmm.hpp>


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