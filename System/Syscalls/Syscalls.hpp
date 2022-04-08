#include <kern_result.hpp>
#include <kern_datatypes.hpp>

#ifndef _SYSCALLS_HPP_
#define _SYSCALLS_HPP_

namespace pantheon
{


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
Result SVCCreatePort(pantheon::TrapFrame *CurFrame);

UINT64 SyscallCount();
BOOL CallSyscall(UINT32 Index, pantheon::TrapFrame *Frame);

}

#endif