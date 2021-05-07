#include <kern_integers.hpp>

#include "DeviceTree.hpp"

#include <kern_runtime.hpp>

bool CheckHeader(fdt_header *Header)
{
	/* Specification mandates this be aligned to 8 bytes. */
	if (((uintptr_t)Header) & 0x07)
	{
		return false;
	}

	return Header->magic.GetNumHost() == 0xd00dfeed;
}


DeviceTreeBlob::DeviceTreeBlob(fdt_header *Header)
{
	this->StructIndex = 0;
	this->rsmvm_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_mem_rsvmap.GetNumHost()));
	this->strings_ptr = (((CHAR*)Header) + (Header->off_dt_strings.GetNumHost()));
	this->struct_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_dt_struct.GetNumHost()));	
}

DeviceTreeBlob::~DeviceTreeBlob()
{

}

void DeviceTreeBlob::NextStruct()
{
	if (this->EndStruct())
	{
		return;
	}

	FDTNodeType CurType = (FDTNodeType)(this->struct_ptr[this->StructIndex].GetNumHost());
	this->StructIndex++;
	switch (CurType)
	{
		case FDT_BEGIN_NODE:
		{
			CHAR *StringBuf = (CHAR*)(this->struct_ptr);
			StringBuf += (sizeof(BEIntegerU32) * this->StructIndex);

			UINT64 Index = 0;
			UINT64 StrLen = 0;

			for (Index = 0; StringBuf[Index] != '\0'; ++Index)
			{
				StrLen++;
			}

			/* Align to 4 bytes */
			StrLen += 4 - (StrLen % 4);
			this->StructIndex += (StrLen / 4);
			break;
		}

		case FDT_END_NODE:
		case FDT_END:
		case FDT_NOP:
		{
			break;
		}

		case FDT_PROP:
		{
			/* The header here is definitely at least 2 long. */
			BEIntegerU32 CurSize = this->struct_ptr[this->StructIndex];
			this->StructIndex += 2;

			UINT32 FinalSpace = CurSize.GetNumHost();
			if (FinalSpace % 4)
			{
				FinalSpace += 4 - (FinalSpace % 4);
			}
			this->StructIndex += FinalSpace / 4;
			break;
		}
	}
}

BOOL DeviceTreeBlob::EndStruct()
{
	return this->struct_ptr[this->StructIndex].GetNumHost() == FDT_END;
}

UINT64 DeviceTreeBlob::GetStructIndex()
{
	return this->StructIndex;
}

FDTNodeType DeviceTreeBlob::GetStructType()
{
	return static_cast<FDTNodeType>(this->struct_ptr[this->StructIndex].GetNumHost());
}