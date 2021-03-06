#ifndef _ARCH_HPP_
#define _ARCH_HPP_

#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#include "vmm/vmm.hpp"

typedef struct PagingInfo
{
	UINT64 TTBR0;
	UINT64 TTBR1;
}PagingInfo;

namespace pantheon
{

typedef pantheon::arm::CpuContext CpuContext;
typedef pantheon::arm::TrapFrame TrapFrame;

inline VOID RearmSystemTimer(UINT64 Freq)
{
	pantheon::arm::RearmSystemTimer(Freq);
}

inline VOID DisableSystemTimer()
{
	pantheon::arm::DisableSystemTimer();
}

inline VOID RearmSystemTimer()
{
	pantheon::arm::RearmSystemTimer();
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

VOID PAUSE();

}

}

extern "C" void enable_interrupts();

#endif