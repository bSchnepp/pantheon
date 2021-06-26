#include "kern_status.hpp"
#include "kern_datatypes.hpp"

#include <Common/Sync/kern_atomic.hpp>

static pantheon::Atomic<pantheon::KernelStatus> CurStatus;

void pantheon::SetKernelStatus(pantheon::KernelStatus Status)
{
	CurStatus.Store(Status);
}

pantheon::KernelStatus pantheon::GetKernelStatus()
{
	return CurStatus.Load();
}