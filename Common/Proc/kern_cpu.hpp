#include <kern_datatypes.hpp>

#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

#ifndef _KERN_CPU_HPP_
#define _KERN_CPU_HPP_

namespace pantheon
{

namespace CPU
{

/**
 * \~english @brief The per-core data structures used by the kernel
 * \~english @details A structure holding onto the specific details of a
 * current processor. This struct is analagous to struct cpu in xv6.
 * 
 * This struct contains information about the current processor such as the
 * current thread it's executing, and the process belonging to that thread.
 * Likewise, a reference is also available to the scheduler of the current
 * processor, so that additional tasks can be queued and executed for this
 * processor core.
 * 
 * Naturally, the contents of this struct may change over time, but those
 * items will always be present.
 * 
 * \~english @author Brian Schnepp
 */
typedef struct CoreInfo
{
	pantheon::Thread *CurThread;
	pantheon::Scheduler *CurSched;
}CoreInfo;

void InitCoreInfo(UINT8 CoreNo);
CoreInfo *GetCoreInfo();

UINT8 GetProcessorNumber();

GlobalScheduler *GetGlobalScheduler();

}

}

#endif