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
Result SVCLogText(const CHAR *Data);
Result SVCAllocateBuffer(UINT64 Sz);

}

#endif