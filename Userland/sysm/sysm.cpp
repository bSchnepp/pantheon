#include <SDK/pantheon.h>

void sysm_Main()
{
	svc_LogText("system manager started");

	INT32 Read;
	INT32 Write;

	INT32 ServerPortRegistration;
	INT32 ClientPortRegistration;

	pantheon::Result Status = svc_CreatePort("sysm:reg", 64, &ServerPortRegistration, &ClientPortRegistration);
	if (Status != pantheon::Result::SYS_OK)
	{
		svc_LogText("sysm:reg port creation failed. ABORT!");
		for(;;){}
	}

	/* TODO: Accept data from it... */
	INT32 ServerConnection;
	Status = svc_AcceptConnection(ServerPortRegistration, &ServerConnection);
	if (Status == pantheon::Result::SYS_FAIL)
	{
		svc_LogText("Unable to accept a session");
	}
	else
	{
		for (;;)
		{
			/* Reply with nothing, forever. */
			svc_ReplyAndRecieve(0, nullptr, &ServerConnection, 1000);
		}
	}
	

	Status = svc_CreateNamedEvent("signal", &Read, &Write);
	if (Status != pantheon::Result::SYS_OK)
	{
		svc_LogText("signal creation failed. ABORT!");
		for(;;){}
	}

	for (;;)
	{
		INT32 ClientConn;
		Status = svc_ConnectToNamedPort("sysm:reg", &ClientConn);
		if (Status != pantheon::Result::SYS_OK)
		{
			svc_LogText("Cannot connect to sysm:reg!");
		}
		else 
		{
			svc_SignalEvent(Write);
			svc_CloseHandle(ClientConn);
		}
		svc_LogText("IN USERSPACE [sysm]");
	}
}

extern "C"
{

uintptr_t __stack_chk_guard = 0xDEADBEEFDEADC0DE;

[[noreturn]] VOID __stack_chk_fail(void)
{
	svc_LogText("CRASH: sysm has stack canary smashed.\n");
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