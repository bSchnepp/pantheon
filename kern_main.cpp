#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <DeviceTree/DeviceTree.hpp>

constexpr UINT32 ConstStrLen(const CHAR *Str)
{
	UINT32 Count = 0;
	for (Count = 0; Str[Count] != '\0'; ++Count)
	{
	}
	return Count;
}

int32_t MemCmp(CHAR *Arg1, CHAR *Arg2, UINT32 Amt)
{
	for (UINT32 Index = 0; Index < Amt; ++Index)
	{
		if (Arg1[Index] != Arg2[Index])
		{
			return Index;
		}

		if (Arg1[Index] == 0)
		{
			return Index;
		}
	}
	return 0;
}

void ClearBuffer(CHAR *Location, UINT32 Amount)
{
	for (UINT32 Index = 0; Index < Amount; ++Index)
	{
		Location[Index] = '\0';
	}
}

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init(fdt_header *dtb)
{
	volatile bool CheckMe = CheckHeader(dtb);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	DeviceTreeBlob DTBState(dtb);

	BoardInit();
	WriteString("booting based on device tree pointer!\n");

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
			DTBState.CopyStringFromOffset(Offset, Buffer, 512);
			WriteString(CurDevNode);
			WriteString(" : ");
			WriteString(Buffer);
			if (IsStringPropType(Buffer) || IsStringListPropType(Buffer))
			{
				CHAR Buffer2[512];
				DTBState.CopyStringFromStructPropNode(Buffer2, 512);
				WriteString(" (");
				WriteString(Buffer2);
				WriteString(")");
			}
			WriteString("\n");
		}
		else if (CurNode == FDT_BEGIN_NODE)
		{
			ClearBuffer(CurDevNode, 512);
			DTBState.CopyStringFromStructBeginNode(CurDevNode, 512);
			WriteString("<<");
			WriteString(CurDevNode);
			WriteString(">>\n");
		}
	}

	WriteString("finished going through dtb");

	for (;;)
	{
	}
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
