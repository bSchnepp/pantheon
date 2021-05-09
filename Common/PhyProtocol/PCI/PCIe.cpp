#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include "PCIe.hpp"

static void *PCIeAddr;

void pantheon::pcie::InitPCIe(void *Addr)
{
	PCIeAddr = Addr;
}