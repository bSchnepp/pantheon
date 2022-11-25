#include <vmm/pte.hpp>
#include <kern_datatypes.hpp>

#include <System/Exec/kern_proc_alsr.hpp>

pantheon::vmm::VirtualAddress pantheon::GenerateALSRBase()
{
	/* TODO: Grab some real randomness. This is still
	 * deterministic, but that's probably fine (for now).
	 */
	static pantheon::vmm::VirtualAddress Seed = pantheon::ALSRMaxAddress;
	
	/* Arbitrary constants */
	Seed *= 0xBF83525;
	Seed += 0x8DD1FC7;

	return (Seed % pantheon::ALSRMaxAddress) & ~(pantheon::ALSRMask - 1);
}