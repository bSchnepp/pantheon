#include "DeviceTree.hpp"

#include <kern_runtime.hpp>
#include <kern_integers.hpp>

/**
 * @file Common/DeviceTree/DeviceTree.cpp
 * \~english @brief Definitions for management of the device tree blob present
 * on a device, after initialization by underlying firmware (ie, Das-U-Boot)
 * \~english @author Brian Schnepp
 */

/**
 * \~english @brief Checks the validity of the DeviceTree header, ensuring
 * that it correctly follows the specification with alignment and it's magic number.
 * \~english @author Brian Schnepp
 * \~english @param Header The location of the DeviceTree flattened binary header
 */
bool CheckHeader(fdt_header *Header)
{
	/* Specification mandates this be aligned to 8 bytes. */
	if (((uintptr_t)Header) & 0x07)
	{
		return false;
	}

	return Header->magic.GetNumHost() == 0xd00dfeed;
}

/**
 * \~english @brief Initializes a class representing the state of traversal down the
 * device tree blob structure. All accesses to the DeviceTree are done as read-only,
 * such that allocations of multiple DeviceTreeBlob objects are thread-safe.
 * \~english @author Brian Schnepp
 * \~english @param Header The location of the DeviceTree flattened binary header. Header is assumed to be valid, as per CheckHeader
 * @see CheckHeader
 */
DeviceTreeBlob::DeviceTreeBlob(fdt_header *Header)
{
	this->StructIndex = 0;
	this->AddressCellsAmt = 0;
	this->SizeCellsAmt = 0;

	this->rsmvm_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_mem_rsvmap.GetNumHost()));
	this->strings_ptr = (((CHAR*)Header) + (Header->off_dt_strings.GetNumHost()));
	this->struct_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_dt_struct.GetNumHost()));
}

DeviceTreeBlob::~DeviceTreeBlob()
{

}

/**
 * \~english @brief Iterates the current DeviceTreeBlob structure to move to the
 * next entry in the flattened device tree.
 * \~english @author Brian Schnepp
 * \~english @param Header The location of the DeviceTree flattened binary header. Header is assumed to be valid, as per CheckHeader
 * @see CheckHeader
 */
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

/**
 * \~english @brief Checks if the current iterator has reached the end of the
 * device tree. Per the DeviceTree specification, this is true if and only if
 * the last node eached is FDT_END.
 * \~english @author Brian Schnepp
 * \~english @returns True if there is any more nodes in the DTB to process,
 * false if FDT_END was reached.
 * @see FDT_END
 */
BOOL DeviceTreeBlob::EndStruct()
{
	return this->struct_ptr[this->StructIndex].GetNumHost() == FDT_END;
}

/**
 * \~english @brief Obtains the raw pointer offset used for the DeviceTree blob.
 * It is important to note that this is not the index of the current node being
 * processed, but is instead of location from the start of the structure pointer
 * that is currently active.
 * \~english @author Brian Schnepp
 * \~english @returns An offset from the start of the DeviceTree that is currently
 * being parsed.
 */
[[nodiscard]]
UINT64 DeviceTreeBlob::GetStructIndex() const
{
	return this->StructIndex;
}

/**
 * \~english @brief Obtains the current FDT node type from the currently
 * active DeviceTree node. This can be one of any valid type of FDTNodeType.
 * \~english @author Brian Schnepp
 * \~english @returns The currently selected FDT node type.
 * @see FDTNodeType
 */
[[nodiscard]]
FDTNodeType DeviceTreeBlob::GetStructType() const
{
	return static_cast<FDTNodeType>(this->struct_ptr[this->StructIndex].GetNumHost());
}

/**
 * \~english @brief Obtains the offset into the strings table where the name of
 * the currently active struct is located. This function is only valid if the
 * currently active FDT node type is an FDT_PROP.
 * \~english @author Brian Schnepp
 * \~english @returns The offset of the strings table where the property name is
 * located. If not valid, then 0 is returned.
 * @see FDTNodeType
 * @see GetStructType
 */
UINT64 DeviceTreeBlob::GetPropStructNameIndex()
{
	if (this->struct_ptr[this->StructIndex].GetNumHost() != FDT_PROP)
	{
		return 0;
	}

	return this->struct_ptr[this->StructIndex + 2].GetNumHost();
}

/**
 * \~english @brief Copies a string from the string table, given an offset,
 * into another user-supplied buffer. If the null string is reached, or the
 * buffer given is filled, then the function terminates filling the buffer.
 * \~english @details The resulting string will be copied up to the end of the
 * buffer, or the end of the string. Should the given string exceed the buffer
 * size, the string will be terminated at that location, and the null character
 * is placed at the end, ensuring strings are always null-terminated.
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyStringFromOffset(UINT64 Offset, CHAR *Buffer, UINT64 BufferSize)
{
	UINT64 CurrentAmt = 0;
	for (CurrentAmt = 0; CurrentAmt < BufferSize; ++CurrentAmt)
	{
		Buffer[CurrentAmt] = this->strings_ptr[Offset + CurrentAmt];
		if (Buffer[CurrentAmt] == '\0')
		{
			break;
		}
	}
	Buffer[BufferSize - 1] = '\0';
}

/**
 * \~english @brief Copies a string from the current begin node into another buffer.
 * \~english @details The resulting string will be copied up to the end of the
 * buffer, or the end of the string. Should the given string exceed the buffer
 * size, the string will be terminated at that location, and the null character
 * is placed at the end, ensuring strings are always null-terminated.
 * 
 * This function will not modify the supplied buffer if the current node is not
 * an FDT_BEGIN_NODE. Thus, this function should only be used if the current
 * node is an FDT_BEGIN_NODE.
 * 
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyStringFromStructBeginNode(CHAR *Buffer, UINT64 BufferSize)
{
	if (this->struct_ptr[this->StructIndex].GetNumHost() != FDT_BEGIN_NODE)
	{
		return;
	}

	CHAR *StringBuf = (CHAR*)(this->struct_ptr);
	StringBuf += (sizeof(BEIntegerU32) * (this->StructIndex + 1));

	UINT64 CurrentAmt = 0;
	for (CurrentAmt = 0; CurrentAmt < BufferSize; ++CurrentAmt)
	{
		Buffer[CurrentAmt] = StringBuf[CurrentAmt];
		if (Buffer[CurrentAmt] == '\0')
		{
			break;
		}
	}
	Buffer[BufferSize - 1] = '\0';

}

/**
 * \~english @brief Copies a string from the current property into another buffer.
 * \~english @details The resulting string will be copied up to the end of the
 * buffer, or the end of the string. Should the given string exceed the buffer
 * size, the string will be terminated at that location, and the null character
 * is placed at the end, ensuring strings are always null-terminated.
 * 
 * This function will not modify the supplied buffer if the current node is not
 * an FDT_PROP. Thus, this function should only be used if the current
 * node is an FDT_PROP.
 * 
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyStringFromStructPropNode(CHAR *Buffer, UINT64 BufferSize)
{
	if (this->struct_ptr[this->StructIndex].GetNumHost() != FDT_PROP)
	{
		return;
	}

	CHAR *StringBuf = (CHAR*)(this->struct_ptr);
	StringBuf += (sizeof(BEIntegerU32) * (this->StructIndex + 3));

	UINT64 CurrentAmt = 0;
	for (CurrentAmt = 0; CurrentAmt < BufferSize; ++CurrentAmt)
	{
		Buffer[CurrentAmt] = StringBuf[CurrentAmt];
		if (Buffer[CurrentAmt] == '\0')
		{
			break;
		}
	}
	Buffer[BufferSize - 1] = '\0';
}

/**
 * \~english @brief Copies a u32 from the current property into another buffer. 
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyU32FromStructPropNode(UINT32 *Buffer)
{
	this->CopyU32FromStructPropNode(Buffer, 0);
}

/**
 * \~english @brief Copies a u64 from the current property into another buffer. 
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyU64FromStructPropNode(UINT64 *Buffer)
{
	this->CopyU64FromStructPropNode(Buffer, 0);
}

/**
 * \~english @brief Copies a u32 from the current property into another buffer. 
 * \~english @param[in] Buffer The UINT32 to copy to
 * \~english @param[in] Offset The offset, as an index in array, to copy from the current node
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyU32FromStructPropNode(UINT32 *Buffer, UINT32 Offset)
{
	if (this->struct_ptr[this->StructIndex].GetNumHost() != FDT_PROP)
	{
		return;
	}

	CHAR *RawBuf = (CHAR*)(this->struct_ptr);
	RawBuf += (sizeof(BEIntegerU32) * (this->StructIndex + (3 + Offset)));

	BEIntegerU32 *AsU32 = (BEIntegerU32*)RawBuf;
	*Buffer = AsU32->GetNumHost();
}

/**
 * \~english @brief Copies a u64 from the current property into another buffer. 
 * \~english @param[in] Buffer The UINT64 to copy to
 * \~english @param[in] Offset The offset, as an index in array, to copy from the current node
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::CopyU64FromStructPropNode(UINT64 *Buffer, UINT32 Offset)
{
	if (this->struct_ptr[this->StructIndex].GetNumHost() != FDT_PROP)
	{
		return;
	}

	CHAR *RawBuf = (CHAR*)(this->struct_ptr);
	RawBuf += (sizeof(BEIntegerU32) * (this->StructIndex + (3 + Offset)));

	BEIntegerU64 *AsU64 = (BEIntegerU64*)RawBuf;
	*Buffer = AsU64->GetNumHost();
}

/**
 * \~english @brief Parses a node begin type name to a node name and unit address.
 * \~english @details The supplied node name should be obtained from FDT_BEGIN_NODE.
 * Per the DeviceTree Specification, the string supplied as data for that node
 * must be of the form `node-name@unit-address`, and if such a node name or address
 * does not exist, '/' is used instead. Should it have no reg property inside, the
 * `@unit-address` portion is omitted. This function, appropriately, handles the
 * cases supplied by writing a zero to the address if no address is supplied, and
 * writes "/" to the DeviceType if no type is defined as well.
 * \~english @author Brian Schnepp
 * @see FDT_BEGIN_NODE
 */
void DeviceTreeBlob::NodeNameToAddress(CHAR *Buffer, CHAR *DeviceType, UINT64 DeviceTypeBufferSpace, UINT64 *Address)
{
	/* Can't do anything really. */
	if (DeviceTypeBufferSpace < 2)
	{
		return;
	}

	UINT64 StrLen = 0;
	UINT64 AtLocation = 0;
	for (UINT64 Index = 0; Buffer[Index] != '\0'; ++Index)
	{
		StrLen++;
		if (Buffer[Index] == '@')
		{
			AtLocation = Index;
		}
	}

	for (UINT64 NameIndex = 0; NameIndex < DeviceTypeBufferSpace; ++NameIndex)
	{
		if (AtLocation && NameIndex >= AtLocation)
		{
			DeviceType[NameIndex] = '\0';
			break;
		}
		DeviceType[NameIndex] = Buffer[NameIndex];
	}
	DeviceType[DeviceTypeBufferSpace - 1] = '\0';

	*Address = 0;
	if (AtLocation)
	{
		*Address = CharStarNumberAtoiB16<UINT64>(Buffer + AtLocation + 1);
	}
}

/**
 * \~english @brief Obtains the size specified in the address-cells block of the DTB
 * \~english @details The address-cells block defines the number of 32-bit cells which
 * define the address in any reg field. On a 64-bit system, this is typically 2.
 * If this was not yet processed, the function will return 0.
 * \~english @return The number of cells needed to represent an address, or 0 if not yet known.
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 DeviceTreeBlob::AddressCells() const
{
	return this->AddressCellsAmt;
}

/**
 * \~english @brief Obtains the size specified in the size-cells block of the DTB
 * \~english @details The size-cells block defines the number of 32-bit cells which
 * define the size in any reg field. This defines the size of the area of the DTB,
 * in bytes. 
 * If this was not yet processed, the function will return 0.
 * \~english @return The number of cells needed to represent an address, or 0 if not yet known.
 * \~english @author Brian Schnepp
 */
[[nodiscard]]
UINT64 DeviceTreeBlob::SizeCells() const
{
	return this->SizeCellsAmt;
}

/**
 * \internal
 * \~english @brief Sets the address cells amount for this DTB parser.
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::SetAddressCells(UINT64 Amt)
{
	this->AddressCellsAmt = Amt;
}

/**
 * \internal
 * \~english @brief Sets the size cells amount for this DTB parser.
 * \~english @author Brian Schnepp
 */
void DeviceTreeBlob::SetSizeCells(UINT64 Amt)
{
	this->SizeCellsAmt = Amt;
}


constexpr const CHAR *StringPropTypes[] =
{
	"model",
	"status",
	"bootargs",
	"stdout-path",
	"stdin-path",
	"device_type",
	"power-isa-version",
	"mmu-type",
	"label",
	"phy-connection-type",
};

constexpr const CHAR *StringListPropTypes[] =
{
	"compatible",
	"enable-method",
};

constexpr const CHAR *UInt32PropTypes[] =
{
	"phandle",
	"#address-cells",
	"#size-cells",
	"#interrupt-cells",
	"virtual-reg",
};

constexpr const CHAR *UInt64PropTypes[] =
{
	"phandle",
	"#address-cells",
	"#size-cells",
	"#interrupt-cells",
	"virtual-reg",
};

BOOL IsStringPropType(const CHAR *Prop)
{
	for (const CHAR *Item : StringPropTypes)
	{
		if (StringCompare(Item, Prop, 32))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL IsStringListPropType(const CHAR *Prop)
{
	for (const CHAR *Item : StringListPropTypes)
	{
		if (StringCompare(Item, Prop, 32))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL IsU32PropType(const CHAR *Prop)
{
	for (const CHAR *Item : UInt32PropTypes)
	{
		if (StringCompare(Item, Prop, 32))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL IsU64PropType(const CHAR *Prop)
{
	for (const CHAR *Item : UInt64PropTypes)
	{
		if (StringCompare(Item, Prop, 32))
		{
			return TRUE;
		}
	}
	return FALSE;
}

