#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>
#include <arch/aarch64/cpureg.hpp>

#include <arch/aarch64/sync.hpp>


#ifndef _ARCH_HPP_
#define _ARCH_HPP_

namespace pantheon::ipc
{
	struct ThreadLocalRegion;

	FORCE_INLINE ThreadLocalRegion *GetThreadLocalRegion()
	{
		return reinterpret_cast<ThreadLocalRegion*>(pantheon::CPUReg::R_TPIDRRO_EL0());
	}

	FORCE_INLINE void SetThreadLocalRegion(UINT64 Value)
	{
		pantheon::CPUReg::W_TPIDRRO_EL0(Value);
	}
}

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

FORCE_INLINE UINT64 GetSystemTimerClock()
{
	return pantheon::arm::GetSystemTimerClock();
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

VOID PAUSE();
VOID LIDT(void *IDT);

}



}

extern "C" void enable_interrupts();

#endif