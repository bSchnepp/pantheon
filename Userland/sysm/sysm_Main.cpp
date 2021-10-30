#include <SDK/pantheon.h>

void spawned_thread(void *Unused)
{
	(void)Unused;

	for (;;)
	{
		svc_LogText("IN SPAWNED THREAD");
	}
}

extern "C" void sysm_Main()
{
	svc_LogText("system manager started");
	void *Buffer = svc_AllocateBuffer(4096);

	svc_CreateThread(spawned_thread, nullptr, (char*)Buffer + 4096, 3);
	for (;;)
	{
		svc_LogText("IN USERSPACE");
	}
}