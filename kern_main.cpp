#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>
#include <System/Exec/kern_initialprograms.hpp>

#include <Boot/Boot.hpp>


extern "C" void sysm_Main();
extern "C" void prgm_Main();

void kern_idle2(void *unused)
{
	PANTHEON_UNUSED(unused);
	pantheon::CPU::DropToUsermode((UINT64)sysm_Main);
	for (;;)
	{
		SERIAL_LOG("%s\n", "STUCK IN KERNEL SPACE");
	}
}

void kern_idle3(void *unused)
{
	PANTHEON_UNUSED(unused);
	pantheon::CPU::DropToUsermode((UINT64)prgm_Main);
	for (;;)
	{
		SERIAL_LOG("%s\n", "STUCK IN KERNEL SPACE");
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

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_SECOND_STAGE)
	{
		/* Loop until core 0 finished essential kernel setup */
	}

	pantheon::CPU::InitCoreInfo(CpuNo);
	PerCoreInit();
	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished kernel setup */
	}

	pantheon::RearmSystemTimer(1000);
	pantheon::CPU::STI();
}

void kern_init(InitialBootInfo *InitBootInfo, void *initial_load_addr, void *virt_load_addr)
{
	PANTHEON_UNUSED(InitBootInfo);
	PANTHEON_UNUSED(virt_load_addr);
	PANTHEON_UNUSED(initial_load_addr);
	
	if (pantheon::CPU::GetProcessorNumber() == 0)
	{
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_INIT);
		pantheon::InitBasicRuntime();
		pantheon::InitBasicMemory();
		pantheon::ipc::InitEventSystem();
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);
		/* Create an extra idle thread to ensure rescheduling happens.
		 * Without a spare thread, no scheduling ever occurs. FIXME!
		 */
		pantheon::GetGlobalScheduler()->Init();
		pantheon::UnpackInitPrograms();
		pantheon::GetGlobalScheduler()->CreateProcess("sysm", (void*)kern_idle2);
		pantheon::GetGlobalScheduler()->CreateProcess("prgm", (void*)kern_idle3);

		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
	for (;;)
	{
		pantheon::CPU::GetCoreInfo()->CurSched->Reschedule();
	}	
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
