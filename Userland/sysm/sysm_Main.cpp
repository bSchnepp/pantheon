#include <SDK/pantheon.h>

extern "C" void sysm_Main()
{
	svc_LogText("system manager started");
	for (;;)
	{
		svc_LogText("IN USERSPACE");
	}
}