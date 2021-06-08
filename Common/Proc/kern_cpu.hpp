#include <kern_datatypes.hpp>
#include <Proc/kern_sched.hpp>

#ifndef _KERN_CPU_HPP_
#define _KERN_CPU_HPP_

namespace pantheon
{

namespace CPU
{

/**
 * \~english @brief A description of the current status of the processor
 * \~english @details A given processor in a system running pantheon may be
 * in one of many states. For the OS itself, it largely only matters if this
 * core should be ignored (CPU_STATE_DISABLED), if it's idle (CPU_STATE_IDLE),
 * or if it's actively doing something in either user or kernel mode 
 * (CPU_STATE_ACTIVE_KERN, CPU_STATE_ACTIVE_USER). The global thread scheduler
 * can take this information and appropriately decide what to do with a given
 * core, such as assign another kernel-level thread to avoid jumping to a
 * different permission level, or to stop queueing new jobs so the processor
 * can be put to idle and eventually disabled.
 * 
 * \~english @author Brian Schnepp
 */
typedef enum CPUState : UINT8
{
	CPU_STATE_DISABLED = 0,
	CPU_STATE_IDLE = 1,
	CPU_STATE_ACTIVE_KERN = 2,
	CPU_STATE_ACTIVE_USER = 3,
}CPUState;

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
	CPUState CurState;

	pantheon::Thread *CurThread;
	pantheon::Process *CurProcess;
	pantheon::Scheduler *CurSched;
}CoreInfo;

void InitCoreInfo(UINT8 CoreNo);
CoreInfo *GetCoreInfo();


UINT8 GetProcessorNumber();

}

}

#endif