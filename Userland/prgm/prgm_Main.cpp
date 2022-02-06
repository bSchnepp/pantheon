#include <SDK/pantheon.h>


extern "C" void prgm_Main()
{
	svc_LogText("program manager started");

	UINT8 Read;
	UINT8 Write;

	svc_CreateNamedEvent("signal", &Read, &Write);
	for (;;)
	{
		volatile BOOL Recieve = svc_PollEvent(Read);
		if (Recieve)
		{
			svc_ResetEvent(Read);
			svc_LogText("GOT SIGNAL");
			svc_Yield();
		}
		svc_LogText("IN USERSPACE [prgm]");
	}
}