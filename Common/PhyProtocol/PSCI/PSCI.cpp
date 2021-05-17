#include <kern_runtime.hpp>

#include <kern_datatypes.hpp>
#include <PhyProtocol/PSCI/PSCI.hpp>


typedef enum PSCIFunctionID : UINT32
{
	PSCI_VERSION = 0x84000000,
	PSCI_SUSPEND = 0xC4000001,
	PSCI_CPU_OFF = 0x84000002,
	PSCI_CPU_ON = 0xC4000003,
	PSCI_AFFINITY_INFO = 0xC4000004,
	PSCI_SHUTDOWN = 0x84000008,
	PSCI_RESET = 0x84000009,
	PSCI_FEATURES = 0x8400000A,
}PSCIFunctionID;

static psci::PsciState CurDriverState = psci::PSCI_DRIVER_NOT_INIT;

extern "C" INT64 CallSMC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	volatile INT64 Result;
	asm volatile (
		"smc #0\n" 		
		"mov %0, x0\n"
			: "=r"(Result): "r"(X0), "r"(X1), "r"(X2), "r"(X3));
	return Result;
}

extern "C" INT64 CallHVC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	volatile INT64 Result;
	asm volatile (
		"hvc #0\n" 
		"mov %0, x0\n"
			: "=r"(Result): "r"(X0), "r"(X1), "r"(X2), "r"(X3));
	return Result;
}

void psci::PSCIInit()
{
	/* Assume SMC by default. */
	CurDriverState = PSCI_USE_SMC;
}

void psci::PSCISetMethod(psci::PsciState Type)
{
	CurDriverState = Type;
}

UINT32 psci::PSCIVersion()
{
	if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_VERSION, 0, 0, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_VERSION, 0, 0, 0);
	}

	return 0;
}

INT32 psci::PSCISuspend(UINT32 PowerState, UINT64 EntryPoint, UINT64 ContextID)
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return PSCI_NO_DRIVER;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_SUSPEND, PowerState, EntryPoint, ContextID);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_SUSPEND, PowerState, EntryPoint, ContextID);
	}
	return PSCI_NO_DRIVER;
}

INT32 psci::PSCICpuOff()
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return PSCI_NO_DRIVER;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_CPU_OFF, 0, 0, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_CPU_OFF, 0, 0, 0);
	}
	return PSCI_NO_DRIVER;
}

INT32 psci::PSCICpuOn(UINT64 TargetCPU, UINT64 EntryPoint, UINT64 ContextID)
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return PSCI_NO_DRIVER;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_CPU_ON, TargetCPU, EntryPoint, ContextID);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_CPU_ON, TargetCPU, EntryPoint, ContextID);
	}
	return PSCI_NO_DRIVER;
}

INT32 psci::PSCIAffinityInfo(UINT64 TargetAffinity, UINT32 LowestAffinity)
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return PSCI_NO_DRIVER;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_AFFINITY_INFO, TargetAffinity, LowestAffinity, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_AFFINITY_INFO, TargetAffinity, LowestAffinity, 0);
	}
	return PSCI_NO_DRIVER;
}

VOID psci::PSCIShutdown()
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		CallSMC(PSCI_SHUTDOWN, 0, 0, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		CallHVC(PSCI_SHUTDOWN, 0, 0, 0);
	}
}

VOID psci::PSCIReset()
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		CallSMC(PSCI_RESET, 0, 0, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		CallHVC(PSCI_RESET, 0, 0, 0);
	}	
}

INT32 psci::PSCIFeatures(UINT32 ID)
{
	if (CurDriverState == PSCI_DRIVER_NOT_INIT)
	{
		return PSCI_NO_DRIVER;
	}
	else if (CurDriverState == PSCI_USE_SMC)
	{
		return CallSMC(PSCI_FEATURES, ID, 0, 0);
	}
	else if (CurDriverState == PSCI_USE_HVC)
	{
		return CallHVC(PSCI_FEATURES, ID, 0, 0);
	}
	return PSCI_NO_DRIVER;	
}