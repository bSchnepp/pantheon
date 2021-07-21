#include <arch.hpp>
#include <kern_status.hpp>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Devices/kern_drivers.hpp>
#include <PhyProtocol/DeviceTree/DeviceTree.hpp>

#include <Common/Structures/kern_bitmap.hpp>

#ifndef ONLY_TESTING
extern "C" CHAR *kern_begin;
extern "C" CHAR *kern_end;
#else
UINT64 kern_begin = 0;
UINT64 kern_end = 0;
#endif

typedef struct InitialMemoryArea
{
	UINT64 BaseAddress;
	pantheon::RawBitmap Map;
}InitialMemoryArea;

void ProcessMemory(DeviceTreeBlob *CurState)
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

	if (StringCompare(Buffer, ("reg"), 9))
	{
		/* Assume we'll never need more...? */
		UINT32 Values[64 * 64];

		UINT32 SizeCells = CurState->SizeCells();
		if (SizeCells > 64)
		{
			SizeCells = 64;
		}

		UINT32 AddressCells = CurState->AddressCells();
		if (AddressCells > 64)
		{
			AddressCells = 64;
		}

		for (UINT32 TotalOffset = 0; TotalOffset < SizeCells * AddressCells; ++TotalOffset)
		{
			CurState->CopyU32FromStructPropNode(&Values[TotalOffset], TotalOffset);
		}

		UINT64 Address = 0;
		UINT64 Size = 0;

		for (UINT32 Index = 0; Index < AddressCells; ++Index)
		{
			Address <<= sizeof(UINT32);
			Address += Values[Index];
		}

		for (UINT32 Index = 0; Index < SizeCells; ++Index)
		{
			Size <<= sizeof(UINT32);
			Size += Values[AddressCells + Index];
		}
	}
}

void Initialize(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);
	CHAR CurDevNode[512];
	ClearBuffer(CurDevNode, 512);

	while (!DTBState.EndStruct())
	{
		DTBState.NextStruct();
		FDTNodeType CurNode = DTBState.GetStructType();
		if (CurNode == FDT_PROP)
		{
			UINT64 Offset = DTBState.GetPropStructNameIndex();
			CHAR Buffer[512];
			ClearBuffer(Buffer, 512);
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);
			SERIAL_LOG("%s%s%s", CurDevNode, " : ", Buffer);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				ClearBuffer(Buffer2, 512);
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
				SERIAL_LOG("%s%s%s", " (", Buffer2, ")");
			}
			else if (IsU32PropType(Buffer))
			{
				UINT32 U32;
				DTBState.CopyU32FromStructPropNode(&U32);
				SERIAL_LOG("%s%u%s", " (", U32, ")");
			}
			else if (IsU64PropType(Buffer))
			{
				UINT64 U64;
				DTBState.CopyU64FromStructPropNode(&U64);
				SERIAL_LOG("%s%u%s", " (", U64, ")");
			}
			
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			if (StringCompare(DevName, "memory", 7) == TRUE)
			{
				ProcessMemory(&DTBState);
			}
			else
			{
				/* TODO: This should be connecting to a driver init graph... */
				DriverHandleDTB(DevName, &DTBState);
			}
			SERIAL_LOG("%s", "\n");
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
			SERIAL_LOG("%s%s%s", "<<", CurDevNode, ">>\n");

			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			InitDriver(DevName, Addr);

		}
		else if (CurNode == FDT_END_NODE)
		{
			CHAR DevName[512];
			UINT64 Addr;
			DTBState.NodeNameToAddress(CurDevNode, DevName, 512, &Addr);
			FiniDriver(DevName, Addr);
		}
	}
	SERIAL_LOG("%s\n", "finished going through dtb");
}

extern "C" void BootInit(fdt_header *dtb, void *initial_load_addr, void *virt_load_addr)
{
	PANTHEON_UNUSED(initial_load_addr);
	PANTHEON_UNUSED(virt_load_addr);

	pantheon::CPU::CLI();
	if (pantheon::CPU::GetProcessorNumber() == 0)
	{
		BoardInit();
		Initialize(dtb);
	}
}