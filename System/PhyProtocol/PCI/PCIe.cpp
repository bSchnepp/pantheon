#include <kern_integers.hpp>
#include <kern_datatypes.hpp>
#include <kern_runtime.hpp>

#include "PCIe.hpp"

static pantheon::pcie::ECAMTable *PCIeAddr = nullptr;

void pantheon::pcie::InitPCIe(void *Addr)
{
	PCIeAddr = (pantheon::pcie::ECAMTable *)Addr;
}