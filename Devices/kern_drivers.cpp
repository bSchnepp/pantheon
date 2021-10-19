#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#if defined(EnablePL011)
#include <PL011/PL011.hpp>
#endif

#if defined(__aarch64__)
#include <arch/aarch64/gic.hpp>
#endif

#include <Proc/kern_cpu.hpp>

#include <Common/PhyProtocol/PCI/PCIe.hpp>
#include <Common/PhyProtocol/PSCI/PSCI.hpp>
#include <Common/PhyProtocol/DeviceTree/DeviceTree.hpp>

static BOOL UseGIC = FALSE;
static BOOL UseECAM = FALSE;

extern "C" void asm_kern_init_core(UINT64 Stack);

void InitDriver(const CHAR *DriverName, UINT64 Address)
{
	PANTHEON_UNUSED(Address);
	/* For now, only actually do something if we have psci available. */
	if (StringCompare(DriverName, "psci", 5))
	{
		psci::PSCIInit();
	}
}

void DriverHandleDTB(const CHAR *DriverName, DeviceTreeBlob *CurState)
{
	if (StringCompare(DriverName, "psci", 4))
	{
		UINT64 Offset = CurState->GetPropStructNameIndex();
		CHAR Buffer[512];
		CHAR Buffer2[512];
		for (UINT32 Index = 0; Index < 512; ++Index)
		{
			Buffer[Index] = '\0';
			Buffer2[Index] = '\0';
		}
		CurState->CopyStringFromOffset(Offset, Buffer, 512);

		if (StringCompare(Buffer, ("method"), 9))
		{
			CurState->CopyStringFromStructPropNode(Buffer2, 512);
			if (StringCompare(Buffer2, ("smc"), 3))
			{
				psci::PSCISetMethod(psci::PSCI_USE_SMC);
			}
			else if (StringCompare(Buffer2, ("hvc"), 3))
			{
				psci::PSCISetMethod(psci::PSCI_USE_HVC);
			}			
		}
	}
	else if (StringCompare(DriverName, ("intc"), 5))
	{
		UINT64 Offset = CurState->GetPropStructNameIndex();
		CHAR Buffer[512];
		CHAR Buffer2[512];
		for (UINT32 Index = 0; Index < 512; ++Index)
		{
			Buffer[Index] = '\0';
			Buffer2[Index] = '\0';
		}
		CurState->CopyStringFromOffset(Offset, Buffer, 512);

		if (StringCompare(Buffer, ("compatible"), 9))
		{
			CurState->CopyStringFromStructPropNode(Buffer2, 512);
			if (StringCompare(Buffer2, ("arm,cortex-a15-gic"), 18))
			{
				UseGIC = TRUE;
			}
		}		
	}
	else if (StringCompare(DriverName, ("pcie"), 5))
	{
		UINT64 Offset = CurState->GetPropStructNameIndex();
		CHAR Buffer[512];
		CHAR Buffer2[512];
		for (UINT32 Index = 0; Index < 512; ++Index)
		{
			Buffer[Index] = '\0';
			Buffer2[Index] = '\0';
		}
		CurState->CopyStringFromOffset(Offset, Buffer, 512);

		if (StringCompare(Buffer, ("compatible"), 9))
		{
			CurState->CopyStringFromStructPropNode(Buffer2, 512);
			if (StringCompare(Buffer2, ("pci-host-ecam-generic"), 18))
			{
				UseECAM = TRUE;
			}
		}		
	}
}

void FiniDriver(const CHAR *DriverName, UINT64 Address)
{
	/* For now, only actually do something if we have pcie or psci... */
	if (StringCompare(DriverName, "pcie", 4))
	{
		pantheon::pcie::InitPCIe((void*)Address);
	}
	else if (StringCompare(DriverName, "psci", 5))
	{
		for (UINT8 Index = 0; Index < 255; ++Index)
		{
			/* If we have too many CPUs, stop. */
			if (Index > MAX_NUM_CPUS)
			{
				break;
			}

			void *StackPtr = pantheon::CPU::GetStackArea(Index);
			INT32 Result = psci::PSCICpuOn(Index, (UINT64)asm_kern_init_core, (UINT64)StackPtr);
			
			/* Then there are no more CPUs on this node. */
			if (Result == psci::PSCI_INVALID_PARAMETERS)
			{
				break;
			}
		}
	}
}

/* HACK: This should be put in a better place.. */
extern VOID PerCoreBoardInit();

extern char interrupt_table[];

void PerCoreInit()
{
	pantheon::CPU::LIDT(interrupt_table);
	PerCoreBoardInit();
}