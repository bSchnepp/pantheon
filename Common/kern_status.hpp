#include <kern_datatypes.hpp>

#ifndef _KERN_STATUS_HPP_
#define _KERN_STATUS_HPP_

namespace pantheon
{

typedef enum KernelStatus : UINT32
{
	KERNEL_STATUS_ERR = 0,
	KERNEL_STATUS_INIT = 1,
	KERNEL_STATUS_SECOND_STAGE = 2,
	KERNEL_STATUS_OK = 3,
	KERNEL_STATUS_PANIC = 4,
}KernelStatus;

void SetKernelStatus(KernelStatus Status);
KernelStatus GetKernelStatus();

}

#endif