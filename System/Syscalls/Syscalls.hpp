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
VOID SVCExitProcess(pantheon::TrapFrame *CurFrame);
Result SVCForkProcess(pantheon::TrapFrame *CurFrame);
Result SVCLogText(pantheon::TrapFrame *CurFrame);
Result SVCAllocateBuffer(pantheon::TrapFrame *CurFrame);
Result SVCCreateThread(pantheon::TrapFrame *CurFrame);
Result SVCCreateNamedEvent(pantheon::TrapFrame *CurFrame);
Result SVCSignalEvent(pantheon::TrapFrame *CurFrame);
Result SVCClearEvent(pantheon::TrapFrame *CurFrame);
Result SVCResetEvent(pantheon::TrapFrame *CurFrame);
Result SVCPollEvent(pantheon::TrapFrame *CurFrame);
Result SVCYield(pantheon::TrapFrame *CurFrame);
Result SVCExitThread(pantheon::TrapFrame *CurFrame);
Result SVCExecute(pantheon::TrapFrame *CurFrame);
}

#endif