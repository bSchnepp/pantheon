#include <SDK/pantheon.h>

extern "C" void sysm_Main()
{
	svc_LogText("system manager started");

	UINT8 Read;
	UINT8 Write;

	svc_CreateNamedEvent("signal", &Read, &Write);
	for (;;)
	{
		svc_SignalEvent(Write);
		svc_LogText("IN USERSPACE");
	}
}