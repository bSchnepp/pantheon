#include <vmm/pte.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_PROC_ALSR_HPP_
#define _KERN_PROC_ALSR_HPP_

namespace pantheon
{

/* Do not generate any base address over 2GB. That's totally excessive. */
constexpr static pantheon::vmm::VirtualAddress ALSRMaxAddress = 0x80000000;

/* Also make sure assignments are aligned to 2MB: this is much larger than a
 * page, and is probably also the size of a block wherever this gets ported to.
 */
constexpr static pantheon::vmm::VirtualAddress ALSRMask = 0x200000;

pantheon::vmm::VirtualAddress GenerateALSRBase();

}

#endif