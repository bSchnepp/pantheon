#include <vmm/pte.hpp>
#include <kern_datatypes.hpp>

#include <kern_rand.hpp>
#include <System/Exec/kern_proc_alsr.hpp>

pantheon::vmm::VirtualAddress pantheon::GenerateALSRBase()
{
	return (pantheon::Rand() % pantheon::ALSRMaxAddress) & ~(pantheon::ALSRMask - 1);
}