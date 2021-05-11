#include "kern_status.hpp"
#include "kern_datatypes.hpp"

static pantheon::KernelStatus CurStatus;

void pantheon::SetKernelStatus(pantheon::KernelStatus Status)
{
	/* TODO: Lock behind a mutex? */
	CurStatus = Status;
}

pantheon::KernelStatus pantheon::GetKernelStatus()
{
	return CurStatus;
}