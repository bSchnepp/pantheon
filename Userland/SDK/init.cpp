#include "pantheon.h"

extern "C" void main();

extern "C" void _start()
{
	main();
	svc_ExitProcess();
	for (;;) {}
}

ThreadLocalRegion *GetTLS(void)
{
	ThreadLocalRegion *RetVal = nullptr;
	asm volatile ("mrs %0, tpidrro_el0\n" : "=r"(RetVal) ::);
	return RetVal;
}