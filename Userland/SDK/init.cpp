#include "pantheon.h"

extern "C" void main();

extern "C" void _start()
{
	main();
	svc_ExitProcess();
	for (;;) {}
}