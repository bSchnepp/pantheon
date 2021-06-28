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


static pantheon::Spinlock PrintLock;

/* Since we don't use the MMU at all, we need thread-unique storage. */
static UINT64 Count[2000];
void kern_idle(void *unused)
{
	PANTHEON_UNUSED(unused);
	volatile UINT64 TID = pantheon::CPU::GetCoreInfo()->CurThread->ThreadID();
	Count[TID] = 0;
	for (;;)
	{
		Count[TID]++;
		/* preemption has to be disabled when contending for this lock.
		 * Otherwise the thread may get preempted before releasing the
		 * lock, preventing anything else from printing.
		 */
		pantheon::CPU::CLI();
		PrintLock.Acquire();
		SERIAL_LOG_UNSAFE("(%hhu) %s: %u [%ld]\n", pantheon::CPU::GetProcessorNumber(), "idle: ", Count[TID], TID);
		PrintLock.Release();
		pantheon::CPU::STI();

		/* stall for time as a good enough demo to show task switching */
		for (UINT64 Wait = 0; Wait < 600000; ++Wait)
		{
			/* ensure clang doesn't optimize away */
			char c = 10;
			c += 10;
			if (c + Wait == 600000)
			{
				break;
			}
		}
	}
}

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init_core()
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	pantheon::CPU::InitCoreInfo(CpuNo);
	PerCoreInit();

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_SECOND_STAGE)
	{
		/* Loop until core 0 finished essential kernel setup */
	}

	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);
	pantheon::CPU::STI();

	/* Ensure there is always at least the idle proc for this core. */
	pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);
	pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished kernel setup */
	}

	pantheon::RearmSystemTimer(1000);
	pantheon::CPU::GetCoreInfo()->CurSched->SignalReschedule();
	for (;;)
	{
		pantheon::CPU::GetCoreInfo()->CurSched->MaybeReschedule();
	}
}

void kern_init(fdt_header *dtb)
{
	pantheon::CPU::CLI();
	if (pantheon::CPU::GetProcessorNumber() == 0)
	{
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_INIT);

		/* The most basic kernel initialization should be done here. */
		BoardInit();
		Initialize(dtb);
		pantheon::GetGlobalScheduler()->Init();

		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);

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
