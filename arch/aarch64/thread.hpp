#include <kern_datatypes.hpp>

#ifndef _PANTHEON_ARM_THREAD_HPP_
#define _PANTHEON_ARM_THREAD_HPP_

namespace pantheon::arm
{

typedef struct CpuContext
{
	static constexpr UINT64 NumRegs = 32;
	static constexpr UINT64 NumFRegs = 32;

	UINT64 Regs[NumRegs];

	/* On aarch64, these are 32 128-bit registers. We don't have a 128-bit
	 * datatype, so store them as pairs of 64-bit numbers instead.
	 */
	UINT64 FRegs[2 * NumFRegs];

	VOID Wipe()
	{
		for (UINT64 &Item : this->Regs)
		{
			Item = 0;
		}
		for (UINT64 &Item : this->FRegs)
		{
			Item = 0;
		}		
	}

	UINT64 &operator[](UINT64 Index)
	{
		return this->Regs[Index];
	}

	[[nodiscard]] 
	UINT64 RegCount() const
	{
		return NumRegs;
	}

	[[nodiscard]] 
	UINT64 FRegCount() const
	{
		return NumFRegs;
	}

}CpuContext;

}


#endif