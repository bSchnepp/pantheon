#include "vmm.hpp"
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <Common/Structures/kern_slab.hpp>

VOID pantheon::vmm::InvalidateTLB()
{
	/* Smash the TLB: we don't want invalid entries left around. */
	asm volatile(
		"isb\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"dsb sy\n"
		"isb\n" ::: "memory");
}