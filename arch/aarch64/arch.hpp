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

inline VOID DisableSystemTimer()
{
	pantheon::arm::DisableSystemTimer();
}

namespace CPU
{

/**
 * \~english @brief Gets the processor number of the current core
 * \~english @author Brian Schnepp
 */
UINT8 GetProcessorNumber();

VOID CLI();
VOID STI();

}
}

#endif