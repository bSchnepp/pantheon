#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <DeviceTree/DeviceTree.hpp>

static fdt_header *dtb_ptr = nullptr;

static BEIntegerU32 *rsmvm_ptr = nullptr;
static CHAR *strings_ptr = nullptr;
static BEIntegerU32 *struct_ptr = nullptr;

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

void WriteTabs(UINT32 TabAmt)
{
	for (UINT32 Index = 0; Index < TabAmt; ++Index)
	{
		WriteSerialChar('\t');
	}
}

void GetStructEntries()
{
	/* Strings *could* be longer, but don't process those. */
	static constexpr UINT32 StringBufSz = 512;
	static volatile CHAR TmpBuffer[StringBufSz];

	/* Go through the structs pointer. */
	bool HitEnd = false;
	UINT32 Index = 0;
	UINT32 IndentIndex = 0;
	while (!HitEnd)
	{
		/* Each item is aligned to 4 bytes.
		 * Usually, it is just the token ID, but it could
		 * also be token ID + content, in the case of FDT_PROP.
		 * 
		 * Iterate through the list and find them. For now,
		 * set a breakpoint on this function to 
		 */
		UINT32 Current = struct_ptr[Index++].GetNumHost();
		if (Current == FDT_BEGIN_NODE)
		{
			WriteString("<<BEGIN DEVICE PROP>>\n");
			IndentIndex = 1;
		}
		else if (Current == FDT_END_NODE)
		{
			WriteString("<<END DEVICE PROP>>\n");
			IndentIndex = 0;
		}
		if (Current == FDT_PROP)
		{
			/* Clear the old buffer. */
			ClearBuffer((CHAR*)TmpBuffer, StringBufSz);

			/* It's always a struct with 
			 * these 2 entries right after. 
			 */
			UINT32 Length = struct_ptr[Index++].GetNumHost();
			UINT32 StringOffset = struct_ptr[Index++].GetNumHost();
			volatile CHAR *ItemData = reinterpret_cast<CHAR*>(struct_ptr + Index);

			CHAR *CurStrMem = (((CHAR*)(strings_ptr)) + StringOffset);

			UINT32 CopyIndex = 0;
			while (CurStrMem[CopyIndex] && CopyIndex < StringBufSz - 1)
			{
				TmpBuffer[CopyIndex] = CurStrMem[CopyIndex];
				CopyIndex++;
			}

			/* Skip this entry's data. */
			UINT32 Off = 4 - (Length % 4);
			if (Length % 4)
			{
				Index += (Length + Off) / 4;
			}

			WriteTabs(IndentIndex);
			WriteString((const CHAR*)TmpBuffer);
			WriteString(":\n");
			WriteTabs(IndentIndex);

			ClearBuffer((CHAR*)TmpBuffer, StringBufSz);
			for (UINT32 ItemIndex = 0; ItemIndex < Length; ++ItemIndex)
			{
				TmpBuffer[ItemIndex] = ItemData[ItemIndex];
			}
			WriteString((const CHAR*)TmpBuffer);
			WriteString("\n");
		}
		else if (Current == FDT_END)
		{
			WriteString("End of device trees\n");
			IndentIndex = 0;
			HitEnd = true;
		}
	}

}

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init(fdt_header *dtb)
{
	dtb_ptr = dtb;
	volatile bool CheckMe = CheckHeader(dtb_ptr);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	UINT32 DTBSize = dtb_ptr->totalsize.GetNumHost();
	rsmvm_ptr = (BEIntegerU32*)(((CHAR*)dtb_ptr) + (dtb_ptr->off_mem_rsvmap.GetNumHost()));
	strings_ptr = (((CHAR*)dtb_ptr) + (dtb_ptr->off_dt_strings.GetNumHost()));
	struct_ptr = (BEIntegerU32*)(((CHAR*)dtb_ptr) + (dtb_ptr->off_dt_struct.GetNumHost()));

	BoardInit();
	WriteString("booting based on device tree pointer!\n");
	GetStructEntries();

	for (;;)
	{
		/* Ensure CheckMe isn't optimized out */
		volatile int k = ((uint64_t)(dtb)) + CheckMe + DTBSize;
	}
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
