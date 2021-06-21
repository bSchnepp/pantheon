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

	UINT64 PC = 0;

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

	VOID SetPC(UINT64 Val)
	{
		this->PC = Val;
	}

	VOID SetSP(UINT64 Val)
	{
		this->Regs[31] = Val;
	}

	VOID SetArg1(UINT64 Val)
	{
		this->Regs[19] = Val;
	}

	CpuContext &operator=(const CpuContext &Other)
	{
		if (this == &Other)
		{
			return *this;
		}

		for (UINT64 Index = 0; Index < NumRegs; ++Index)
		{
			this->Regs[Index] = Other.Regs[Index];
		}
		for (UINT64 Index = 0; Index < NumFRegs; ++Index)
		{
			this->FRegs[Index] = Other.FRegs[Index];
		}
		this->PC = Other.PC;
		return *this;
	}

}CpuContext;

}


#endif