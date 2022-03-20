#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>
#include <arch/aarch64/cpureg.hpp>

#include <arch/aarch64/sync.hpp>


#ifndef _ARCH_HPP_
#define _ARCH_HPP_

namespace pantheon
{

typedef pantheon::arm::CpuContext CpuContext;
typedef pantheon::arm::TrapFrame TrapFrame;

FORCE_INLINE VOID RearmSystemTimer(UINT64 Freq)
{
	pantheon::arm::RearmSystemTimer(Freq);
}

FORCE_INLINE VOID DisableSystemTimer()
{
	pantheon::arm::DisableSystemTimer();
}

FORCE_INLINE VOID RearmSystemTimer()
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
VOID HLT();
BOOL IF();

VOID PUSHI();
VOID POPI();
UINT64 ICOUNT();

VOID PAUSE();
VOID LIDT(void *IDT);

}



}

extern "C" void enable_interrupts();

#endif