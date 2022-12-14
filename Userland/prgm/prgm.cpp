#include <SDK/pantheon.h>

void prgm_Main()
{
	svc_LogText("program manager started");

	//INT32 Read;
	//INT32 Write;

	//svc_CreateNamedEvent("signal", &Read, &Write);
	for (;;)
	{
		//volatile BOOL Recieve = svc_PollEvent(Read);
		//if (Recieve)
		{
			//svc_ResetEvent(Read);
			svc_LogText("GOT SIGNAL");
			svc_Yield();
		}
		svc_LogText("IN USERSPACE [prgm]");
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
	svc_LogText("prgm began in main");
	prgm_Main();
	svc_LogText("prgm returned from main");
}