#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#if defined(EnablePL011)
#include <PL011/PL011.hpp>
#endif

#include <Common/PhyProtocol/PCI/PCIe.hpp>
#include <Common/PhyProtocol/PSCI/PSCI.hpp>
#include <Common/PhyProtocol/DeviceTree/DeviceTree.hpp>

extern "C" void asm_kern_init_core(UINT64 Stack);

void InitDriver(CHAR *DriverName, UINT64 Address)
{
	if (StringCompare((void*)DriverName, (void*)"pl011", 5))
	{
		/* Currently do nothing! BoardInit should have handled the
		 * uart we're actually going to use...
		 */
	}
	else if (StringCompare((void*)DriverName, (void*)"pcie", 4))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl061", 5))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl031", 5))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"psci", 5))
	{
		psci::PSCIInit();
	}
}

void DriverHandleDTB(CHAR *DriverName, DeviceTreeBlob *CurState)
{
	if (StringCompare(DriverName, (void*)"psci", 4))
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

		if (StringCompare(Buffer, (void*)("method"), 9))
		{
			CurState->CopyStringFromStructPropNode(Buffer2, 512);
			if (StringCompare(Buffer2, (void*)("smc"), 3))
			{
				psci::PSCISetMethod(psci::PSCI_USE_SMC);
			}
			else if (StringCompare(Buffer2, (void*)("hvc"), 3))
			{
				psci::PSCISetMethod(psci::PSCI_USE_HVC);
			}			
		}
	}
}

void FiniDriver(CHAR *DriverName, UINT64 Address)
{
	if (StringCompare((void*)DriverName, (void*)"pl011", 5))
	{
		/* Currently do nothing! BoardInit should have handled the
		 * uart we're actually going to use...
		 */
	}
	else if (StringCompare((void*)DriverName, (void*)"pcie", 4))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl061", 5))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl031", 5))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"psci", 5))
	{
		for (UINT8 Index = 0; Index < 255; ++Index)
		{
			/* 16k stack is a huge amount of space... */
			Optional<void*> MaybeStack = BasicMalloc(16 * 1024);
			if (!MaybeStack.GetOkay())
			{
				break;
			}

			INT32 Result = psci::PSCICpuOn(Index, (UINT64)asm_kern_init_core, (UINT64)(MaybeStack.GetValue()));
			
			/* Then there are no more CPUs on this node. */
			if (Result == psci::PSCI_INVALID_PARAMETERS)
			{
				break;
			}
		}
	}
}