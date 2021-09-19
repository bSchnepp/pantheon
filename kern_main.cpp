#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>

#include <Boot/Boot.hpp>

extern "C" void svc_LogText(const CHAR *Content);
void user_idle()
{
	for (;;)
	{
		svc_LogText("IN USERSPACE");
	}
}


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
		SERIAL_LOG("(%hhu) %s\t%u \t\t[%ld]\n", pantheon::CPU::GetProcessorNumber(), "idle: ", Count[TID], TID);
	}
	for (;;){}
}

void kern_idle2(void *unused)
{
	PANTHEON_UNUSED(unused);
	pantheon::CPU::DropToUsermode((UINT64)user_idle);
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
	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_SECOND_STAGE)
	{
		/* Loop until core 0 finished essential kernel setup */
	}

	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	pantheon::CPU::InitCoreInfo(CpuNo);
	PerCoreInit();

	SERIAL_LOG("Pantheon booted with core %hhu\n", CpuNo);

	/* Ensure there is always at least the idle proc for this core. */
	pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);
	pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle2);

	while (pantheon::GetKernelStatus() < pantheon::KERNEL_STATUS_OK)
	{
		/* Loop until core 0 finished kernel setup */
	}

	pantheon::RearmSystemTimer(1000);
	pantheon::CPU::GetCoreInfo()->CurSched->SignalReschedule();
	pantheon::CPU::STI();
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
		pantheon::GetGlobalScheduler()->Init();
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_SECOND_STAGE);

		/* Create an extra idle thread to ensure rescheduling happens.
		 * Without a spare thread, no scheduling ever occurs. FIXME!
		 */
		pantheon::GetGlobalScheduler()->CreateIdleProc((void*)kern_idle);
		pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_OK);
	}
	kern_init_core();
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
