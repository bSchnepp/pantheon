#include "kern_status.hpp"
#include "kern_datatypes.hpp"

static pantheon::KernelStatus CurStatus;

void pantheon::SetKernelStatus(pantheon::KernelStatus Status)
{
	/* FIXME: Make atomic! */
	CurStatus = Status;
}

pantheon::KernelStatus pantheon::GetKernelStatus()
{
	/* FIXME: Make atomic! */
	return CurStatus;
}