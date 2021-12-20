#include <kern_datatypes.hpp>

#ifndef _PSCI_HPP_
#define _PSCI_HPP_


namespace psci
{

typedef enum PsciErrorCode : INT32
{
	PSCI_SUCCESS = 0,
	PSCI_NOT_SUPPORTED = -1,
	PSCI_INVALID_PARAMETERS = -2,
	PSCI_DENIED = -3,
	PSCI_ALREADY_ON = -4,
	PSCI_ON_PENDING = -5,
	PSCI_INTERNAL_FAILURE = -6,
	PSCI_NOT_PRESENT = -7,
	PSCI_DISABLED = -8,
	PSCI_INVALID_ADDRESS = -9,

	/* Only when PSCI is called when it's not actually loaded. */
	PSCI_NO_DRIVER = -999999999,
}PsciErrorCode;

typedef enum PsciState
{
	PSCI_DRIVER_NOT_INIT = 0,
	PSCI_USE_SMC,
	PSCI_USE_HVC,
}PsciState;

void PSCIInit();
void PSCISetMethod(PsciState Type);

UINT32 PSCIVersion();
INT32 PSCISuspend(UINT32, UINT64, UINT64);
INT32 PSCICpuOff();
INT32 PSCICpuOn(UINT64, UINT64, UINT64);
INT32 PSCIAffinityInfo(UINT64, UINT32);
VOID PSCIShutdown();
VOID PSCIReset();
INT32 PSCIFeatures(UINT32);

}



#endif