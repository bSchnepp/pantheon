#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>
#include <PhyProtocol/DeviceTree/DeviceTree.hpp>

void Initialize(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);
	CHAR CurDevNode[512];
	ClearBuffer(CurDevNode, 512);

	while (!DTBState.EndStruct())
	{
		DTBState.NextStruct();
		FDTNodeType CurNode = DTBState.GetStructType();
		if (CurNode == FDT_PROP)
		{
			UINT64 Offset = DTBState.GetPropStructNameIndex();
			CHAR Buffer[512];
			ClearBuffer(Buffer, 512);
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);
			SERIAL_LOG("%s%s%s", CurDevNode, " : ", Buffer);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				ClearBuffer(Buffer2, 512);
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
				SERIAL_LOG("%s%s%s", " (", Buffer2, ")");
			}
			else if (IsU32PropType(Buffer))
			{
				UINT32 U32;
				DTBState.CopyU32FromStructPropNode(&U32);
				SERIAL_LOG("%s%u%s", " (", U32, ")");
			}
			else if (IsU64PropType(Buffer))
			{
				UINT64 U64;
				DTBState.CopyU64FromStructPropNode(&U64);
				SERIAL_LOG("%s%u%s", " (", U64, ")");
			}
			
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			DriverHandleDTB(DevName, &DTBState);
			SERIAL_LOG("%s", "\n");
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
			SERIAL_LOG("%s%s%s", "<<", CurDevNode, ">>\n");

			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			InitDriver(DevName, Addr);

		}
		else if (CurNode == FDT_END_NODE)
		{
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			FiniDriver(DevName, Addr);
		}
	}
	SERIAL_LOG("%s\n", "finished going through dtb");
}

/* Pantheon can have up to 256 processors in theory.
 * In practice, this should probably be cut down to 8 or 16, which is
 * way more realistic for a SoM I can actually buy. 
 * 256 thread x86 systems barely exist, so it's highly unlikely for any aarch64
 * systems with that many cores or more to exist.
 */
static pantheon::CPU::CoreInfo CoreInfo[256];

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init_core()
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	pantheon::CPU::InitCoreInfo(&(CoreInfo[CpuNo]));

	while (pantheon::GetKernelStatus() != pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished initializing the kernel */
	}
	
	PerCoreInit();
	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);

	for (;;)
	{
	}

}

void kern_init(fdt_header *dtb)
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	if (CpuNo == 0)
	{
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_INIT);
		BoardInit();
		SERIAL_LOG("%s\n", "booting based on device tree pointer!");
		Initialize(dtb);
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);
		pantheon::RearmSystemTimer(1000);
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
