#include <Userland/SDK/pantheon.h>

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

	Status = svc_CreateNamedEvent("signal", &Read, &Write);
	if (Status != pantheon::Result::SYS_OK)
	{
		svc_LogText("signal creation failed. ABORT!");
		for(;;){}
	}

	/* TODO: Accept data from it... */
	for (;;)
	{
		INT32 ServerConnection;
		Status = svc_AcceptConnection(ServerPortRegistration, &ServerConnection);
		if (Status == pantheon::Result::SYS_OK)
		{
			ThreadLocalRegion *Data = GetTLS();
			for (UINT16 Index = 0; Index < 1000; Index++)
			{
				Data->Payload.Data[Index] = 0xAA;
			}
			svc_ReplyAndRecieve(&ServerConnection, 0, nullptr);
			svc_LogText("[sysm] Replying to session");
		}
		else
		{
			svc_LogText("Unable to accept a session");
			svc_Yield();
		}
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