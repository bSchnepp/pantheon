#include <byte_swap.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#ifndef _DEVICE_TREE_HPP_
#define _DEVICE_TREE_HPP_

typedef struct fdt_header
{
	BEIntegerU32 magic;
	BEIntegerU32 totalsize;
	BEIntegerU32 off_dt_struct;
	BEIntegerU32 off_dt_strings;
	BEIntegerU32 off_mem_rsvmap;
	BEIntegerU32 version;
	BEIntegerU32 last_comp_version;
	BEIntegerU32 boot_cpuid_phys;
	BEIntegerU32 size_dt_strings;
	BEIntegerU32 size_dt_struct;
}fdt_header;

typedef struct fdt_reserve_entry
{
	BEIntegerU64 address;
	BEIntegerU64 size;
}fdt_reserve_entry;

/* Note that these are host endianness. */
typedef enum FDTNodeType : UINT32
{
	FDT_BEGIN_NODE = 0x00000001,
	FDT_END_NODE = 0x00000002,
	FDT_PROP = 0x00000003,
	FDT_NOP = 0x00000004,
	FDT_END = 0x00000009,
}FDTNodeType;

typedef struct fdt_prop
{
	BEIntegerU32 len;
	BEIntegerU32 nameoff;
}fdt_prop;

BOOL IsStringPropType(const CHAR *Prop);
BOOL IsStringListPropType(const CHAR *Prop);
BOOL IsU32PropType(const CHAR *Prop);
BOOL IsU64PropType(const CHAR *Prop);

class DeviceTreeBlob
{
public:
	DeviceTreeBlob(fdt_header *Header);
	~DeviceTreeBlob();

	void CopyStringFromOffset(UINT64 Offset, CHAR *Buffer, UINT64 BufferSize);

	void NextStruct();
	BOOL EndStruct();

	[[nodiscard]] UINT64 GetStructIndex() const;
	[[nodiscard]] FDTNodeType GetStructType() const;

	UINT64 GetPropStructNameIndex();
	void CopyStringFromStructBeginNode(CHAR *Buffer, UINT64 BufferSize);

	void CopyStringFromStructPropNode(CHAR *Buffer, UINT64 BufferSize);
	void CopyU32FromStructPropNode(UINT32 *Buffer);
	void CopyU64FromStructPropNode(UINT64 *Buffer);

	void CopyU32FromStructPropNode(UINT32 *Buffer, UINT32 Offset);
	void CopyU64FromStructPropNode(UINT64 *Buffer, UINT32 Offset);

	void NodeNameToAddress(CHAR *Buffer, CHAR *DeviceType, UINT64 DeviceTypeBufferSpace, UINT64 *Address);

	[[nodiscard]] UINT64 AddressCells() const;
	[[nodiscard]] UINT64 SizeCells() const;

	void SetAddressCells(UINT64 Amt);
	void SetSizeCells(UINT64 Amt);

private:
	UINT64 StructIndex;

	UINT64 AddressCellsAmt;
	UINT64 SizeCellsAmt;

	BEIntegerU32 *rsmvm_ptr;
	CHAR *strings_ptr;
	BEIntegerU32 *struct_ptr;
};



bool CheckHeader(fdt_header *Header);

#endif