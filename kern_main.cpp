#include <kern_integers.hpp>
#include <DeviceTree/DeviceTree.hpp>

static fdt_header *dtb_ptr = nullptr;

static BEIntegerU32 *rsmvm_ptr = nullptr;
static char *strings_ptr = nullptr;
static BEIntegerU32 *struct_ptr = nullptr;

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void GetStructEntries(fdt_header *dtb)
{
	/* Strings *could* be longer, but don't process those. */
	static constexpr uint32_t StringBufSz = 512;
	static volatile char TmpBuffer[StringBufSz];

	/* Go through the structs pointer. */
	bool HitEnd = false;
	uint32_t Index = 0;
	while (!HitEnd)
	{
		/* Each item is aligned to 4 bytes.
		 * Usually, it is just the token ID, but it could
		 * also be token ID + content, in the case of FDT_PROP.
		 * 
		 * Iterate through the list and find them. For now,
		 * set a breakpoint on this function to 
		 */
		uint32_t Current = struct_ptr[Index++].GetNumHost();
		if (Current == FDT_PROP)
		{
			/* Clear the old buffer. */
			for (uint32_t ClearIndex = 0; ClearIndex < StringBufSz; ++ClearIndex)
			{
				TmpBuffer[ClearIndex] = '\0';
			}

			/* It's always a struct with 
			 * these 2 entries right after. 
			 */
			uint32_t Length = struct_ptr[Index++].GetNumHost();
			uint32_t StringOffset = struct_ptr[Index++].GetNumHost();

			char *CurStrMem = (((char*)(strings_ptr)) + StringOffset);

			uint32_t CopyIndex = 0;
			while (CurStrMem[CopyIndex] && CopyIndex < StringBufSz - 1)
			{
				TmpBuffer[CopyIndex] = CurStrMem[CopyIndex];
				CopyIndex++;
			}

			/* Skip this entrie's data. */
			uint32_t Off = 4 - (Length % 4);
			if (Length % 4)
			{
				Index += (Length + Off) / 4;
			}
		}
		else if (Current == FDT_END)
		{
			HitEnd = true;
		}
	}

}

void kern_init(fdt_header *dtb)
{
	dtb_ptr = dtb;
	volatile bool CheckMe = CheckHeader(dtb_ptr);
	if (!CheckMe)
	{
		/* Loop forever: can't really do anything. */
		for (;;) {}
	}

	uint32_t DTBSize = dtb_ptr->totalsize.GetNumHost();
	rsmvm_ptr = (BEIntegerU32*)(((char*)dtb_ptr) + (dtb_ptr->off_mem_rsvmap.GetNumHost()));
	strings_ptr = (((char*)dtb_ptr) + (dtb_ptr->off_dt_strings.GetNumHost()));
	struct_ptr = (BEIntegerU32*)(((char*)dtb_ptr) + (dtb_ptr->off_dt_struct.GetNumHost()));

	GetStructEntries(dtb);

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
