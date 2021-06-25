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

void kern_idle(void *unused)
{
	PANTHEON_UNUSED(unused);
	UINT64 Count = 0;
	for (;;)
	{
		SERIAL_LOG_UNSAFE("%s: %u\n", "idle: ", Count++);
	}
}

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init_core()
{
	pantheon::CPU::CLI();

	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	pantheon::CPU::InitCoreInfo(CpuNo);

	while (pantheon::GetKernelStatus() != pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished initializing the kernel */
	}

	PerCoreInit();
	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);

	/* Ensure there is always at least the idle proc for this core. */
	pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);

	pantheon::CPU::STI();
	for (;;)
	{
		pantheon::CPU::GetCoreInfo()->CurSched->MaybeReschedule();
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
		pantheon::GetGlobalScheduler()->Init();
		pantheon::RearmSystemTimer(1000);

		/* Create an extra idle thread to ensure rescheduling happens */
		pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
