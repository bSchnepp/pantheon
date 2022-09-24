#include <stdint.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

extern "C"
{

/* Assume we're ever only going to run on 64-bit systems. 
 * Aarch32 (A-class) is getting rarer, and support has been dropped in kernel
 * mode since the A76 anyway.
 */

uintptr_t __stack_chk_guard = 0xDEADC0DEDEADBEEF;

[[noreturn]] VOID __stack_chk_fail(void)
{
	pantheon::StopErrorFmt("Stop error: stack canary was smashed\n");
}


}