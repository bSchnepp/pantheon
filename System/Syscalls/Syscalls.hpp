#include <kern_thread.hpp>
#include <kern_datatypes.hpp>

#ifndef _SYSCALLS_HPP_
#define _SYSCALLS_HPP_

namespace pantheon
{

typedef UINT64 Result;

/* These will almost certainly get reordered/removed/changed etc.
 * For now, these are necessary since proper resource management doesn't
 * really exist in the kernel as of yet.
 */
VOID SVCExitProcess();
Result SVCForkProcess();
Result SVCLogText();
Result SVCAllocateBuffer();
Result SVCCreateThread();
Result SVCCreateNamedEvent();
Result SVCSignalEvent();
Result SVCClearEvent();
Result SVCResetEvent();
Result SVCPollEvent();
Result SVCYield();
Result SVCExitThread();
Result SVCExecute();
}

#endif