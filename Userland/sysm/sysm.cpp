#include <SDK/pantheon.h>

void sysm_Main()
{
	svc_LogText("system manager started");

	UINT8 Read;
	UINT8 Write;

	INT32 ServerPortRegistration;
	INT32 ClientPortRegistration;

	svc_CreatePort("sysm:reg", 64, &ServerPortRegistration, &ClientPortRegistration);

	/* TODO: Accept data from it... */

	svc_CreateNamedEvent("signal", &Read, &Write);
	for (;;)
	{
		svc_SignalEvent(Write);
		svc_LogText("IN USERSPACE [sysm]");
	}
}

extern "C"
{

uintptr_t __stack_chk_guard = 0xDEADBEEFDEADC0DE;

[[noreturn]] VOID __stack_chk_fail(void)
{
	svc_LogText("CRASH: prgm has stack canary smashed.\n");
	svc_ExitProcess();
	for(;;){}
}

}

extern "C" void main()
{
	svc_LogText("sysm began in main");
	sysm_Main();
	svc_LogText("sysm returned from main");
}