#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Proc/kern_sched.hpp>

#include <Devices/kern_drivers.hpp>
#include <System/PhyMemory/kern_alloc.hpp>
#include <System/Exec/kern_initialprograms.hpp>

#include <Boot/Boot.hpp>
#include <BoardRuntime/BoardRT.hpp>

#include <System/IPC/kern_event.hpp>


static void kern_basic_init(InitialBootInfo *InitBootInfo)
{
	pantheon::InitBasicRuntime();
	pantheon::PageAllocator::InitPageAllocator(InitBootInfo);
	pantheon::InitBasicMemory();
	pantheon::InitProcessTables();
	pantheon::ipc::InitEventSystem();
	pantheon::GlobalScheduler::Init();
}

static void kern_stage2_init()
{
	/* TODO: unpack initial programs properly */
	pantheon::UnpackInitPrograms();
}

extern "C" void kern_init_core()
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_SECOND_STAGE)
	{
		/* Loop until core 0 finished essential kernel setup */
	}

	pantheon::CPU::InitCoreInfo(CpuNo);
	PerCoreInit();

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished kernel setup */
	}
	
	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);
	pantheon::RearmSystemTimer(1000);
	pantheon::CPU::STI();
}


extern "C" void kern_init(InitialBootInfo *InitBootInfo)
{
	if (pantheon::CPU::GetProcessorNumber() == 0)
	{
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_INIT);
		BoardRuntimeInit();
		kern_basic_init(InitBootInfo);
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);
		kern_stage2_init();
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
	for (;;)
	{
		pantheon::CPU::GetCurSched()->Reschedule();
	}
	pantheon::StopError("Broke out of reschedule loop\n");
}
