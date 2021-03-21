#include <kern_integers.hpp>

#include "DeviceTree.hpp"

bool CheckHeader(fdt_header *Header)
{
	return Header->magic.GetNumHost() == 0xd00dfeed;
}