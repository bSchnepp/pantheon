#include <kern.h>
#include <kern_datatypes.hpp>

#ifndef MOCK_ARCH_HPP_
#define MOCK_ARCH_HPP_

typedef struct PagingInfo
{
	UINT64 TTBR0;
	UINT64 TTBR1;
}PagingInfo;

namespace pantheon
{
	struct CpuContext;
}

extern "C" void createprocess_tail();
extern "C" void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset);

namespace pantheon
{

static constexpr UINT64 CpuIRegOffset = 0;

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
		this->Regs[0] = Val;
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

	VOID SetInitContext(UINT64 Fn, UINT64 ArgAddr, UINT64 SP)
	{
		UINT64 TailAddr = (UINT64)(createprocess_tail);
		this->Regs[19] = Fn;
		this->Regs[20] = ArgAddr;
		this->SetSP(SP);
		this->SetPC(TailAddr);
	}

}CpuContext;

typedef struct TrapFrame
{
	UINT64 Regs[31];
	UINT64 PC;
	UINT64 PSTATE;
	UINT64 SP;

	VOID Wipe()
	{
		for (UINT64 &Reg : Regs)
		{
			Reg = 0;
		}
		PC = 0;
		PSTATE = 0;
		SP = 0;
	}
}TrapFrame;

VOID RearmSystemTimer();
VOID RearmSystemTimer(UINT64 Freq);

VOID DisableSystemTimer();

namespace CPU
{

UINT8 GetProcessorNumber();

VOID CLI();
VOID STI();

VOID PUSHI();
VOID POPI();

VOID HLT();
UINT64 ICOUNT();

VOID PAUSE();

}

}

#endif