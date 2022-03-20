#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _ARCH_CPU_REG_HPP_
#define _ARCH_CPU_REG_HPP_


namespace pantheon::CPUReg
{

FORCE_INLINE UINT64 R_TTBR0_EL1()
{
	UINT64 RetVal = 0;
	return RetVal;
}

FORCE_INLINE UINT64 R_TTBR1_EL1()
{
	UINT64 RetVal = 0;
	return RetVal;
}

FORCE_INLINE VOID W_TTBR0_EL1(UINT64 Val)
{
}

FORCE_INLINE VOID W_TTBR1_EL1(UINT64 Val)
{
}

FORCE_INLINE VOID W_MAIR_EL1(UINT64 Value)
{
}

FORCE_INLINE UINT64 R_MAIR_EL1()
{
	UINT64 RetVal = 0;
	return RetVal;
}

FORCE_INLINE VOID W_TCR_EL1(UINT64 Value)
{
}

FORCE_INLINE UINT64 R_TCR_EL1()
{
	UINT64 RetVal = 0;
	return RetVal;
}


}

#endif