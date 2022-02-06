#include <SDK/pantheon.h>

/* Hack until proper loader is ready */
extern "C" void sysm_Main();

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
	sysm_Main();
}