#include <kern_thread.hpp>
#include <kern_datatypes.hpp>

#ifndef _SYSCALLS_HPP_
#define _SYSCALLS_HPP_

namespace pantheon
{

typedef UINT64 Result;
typedef void (*ThreadStartPtr)(void*);

/* These will almost certainly get reordered/removed/changed etc.
 * For now, these are necessary since proper resource management doesn't
 * really exist in the kernel as of yet.
 */
VOID SVCExitProcess();
Result SVCForkProcess();
Result SVCLogText(const CHAR *Data);
Result SVCAllocateBuffer(UINT64 Sz);
Result SVCCreateThread(ThreadStartPtr Entry, VOID *RESERVED, void *StackTop, pantheon::ThreadPriority Priority);
Result SVCCreateNamedEvent(const CHAR *Name, UINT32 *ReadHandle, UINT32 *WriteHandle);
Result SVCSignalEvent(UINT32 WriteHandle);
Result SVCClearEvent(UINT32 WriteHandle);
Result SVCResetEvent(UINT32 ReadHandle);
Result SVCPollEvent(UINT32 Handle);
Result SVCYield();
Result SVCExitThread();
Result SVCExecute(UINT32 Handle);
}

#endif