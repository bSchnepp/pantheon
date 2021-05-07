#include <kern_integers.hpp>

#include "DeviceTree.hpp"

bool CheckHeader(fdt_header *Header)
{
	/* Specification mandates this be aligned to 8 bytes. */
	if (((uintptr_t)Header) & 0x07)
	{
		return false;
	}

	return Header->magic.GetNumHost() == 0xd00dfeed;
}

void InitializeDeviceTreeState(fdt_header *Header, DeviceTreeState *State)
{
	State->AtEnd = false;
	State->Index = 0;
	State->rsmvm_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_mem_rsvmap.GetNumHost()));
	State->strings_ptr = (((CHAR*)Header) + (Header->off_dt_strings.GetNumHost()));
	State->struct_ptr = (BEIntegerU32*)(((CHAR*)Header) + (Header->off_dt_struct.GetNumHost()));	
}


void GetNextDeviceTreeNode(DeviceTreeState &CurState)
{
	return;
}