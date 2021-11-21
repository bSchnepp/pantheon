#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>

#include <Boot/Boot.hpp>


extern "C" void sysm_Main();
extern "C" void prgm_Main();

/* Since we don't use the MMU at all, we need thread-unique storage, 
 * since theres no kernel-level thread-local storage. */
void kern_idle(void *unused)
{
	PANTHEON_UNUSED(unused);
	static UINT64 Count[2000];
	volatile UINT64 TID = pantheon::CPU::GetCurThread()->ThreadID();
	Count[TID] = 0;
	for (;;)
	{
		Count[TID]++;
	}
	for (;;){}
}

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

	volatile UINT64 SCTLRVal = 0;
	asm volatile(
		"mrs %0, sctlr_el1\n"
		"isb\n"
		"dsb sy"
		: "=r"(SCTLRVal):: "memory");

	SERIAL_LOG("Pantheon booted with core %hhu, and paging is %x\n", CpuNo, SCTLRVal & 0x01);

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished kernel setup */
	}

	pantheon::RearmSystemTimer(1000);
	pantheon::CPU::STI();

	pantheon::CPU::GetCoreInfo()->CurSched->SignalReschedule();
	for (;;)
	{
		pantheon::CPU::GetCoreInfo()->CurSched->MaybeReschedule();
	}
}

void kern_init(InitialBootInfo *InitBootInfo, void *initial_load_addr, void *virt_load_addr)
{
	PANTHEON_UNUSED(InitBootInfo);
	PANTHEON_UNUSED(virt_load_addr);
	PANTHEON_UNUSED(initial_load_addr);
	
	if (pantheon::CPU::GetProcessorNumber() == 0)
	{
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_INIT);
		pantheon::InitBasicMemory();
		pantheon::GetGlobalScheduler()->Init();
		pantheon::ipc::InitEventSystem();
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);

		/* Create an extra idle thread to ensure rescheduling happens.
		 * Without a spare thread, no scheduling ever occurs. FIXME!
		 */
		pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle2);
		pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle3);

		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
