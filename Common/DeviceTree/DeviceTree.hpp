#include <kern_datatypes.hpp>
#include <kern_integers.hpp>

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

typedef struct DeviceTreeState
{
	UINT64 Index;
	bool AtEnd;

	void *rsmvm_ptr;
	void *strings_ptr;
	void *struct_ptr;
}DeviceTreeState;



bool CheckHeader(fdt_header *Header);
void InitializeDeviceTreeState(fdt_header *Header, DeviceTreeState *State);
void GetNextDeviceTreeNode(DeviceTreeState &CurState);

#endif