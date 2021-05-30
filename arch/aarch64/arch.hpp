#ifndef _ARCH_HPP_
#define _ARCH_HPP_

#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

namespace pantheon
{

typedef pantheon::arm::CpuContext CpuContext;

inline VOID RearmSystemTimer(UINT64 Freq)
{
	pantheon::arm::RearmSystemTimer(Freq);
}

}

#endif