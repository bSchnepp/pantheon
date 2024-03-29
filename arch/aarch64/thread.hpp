#include <kern_datatypes.hpp>
#include <arch/aarch64/trap.hpp>

#ifndef _PANTHEON_ARM_THREAD_HPP_
#define _PANTHEON_ARM_THREAD_HPP_

extern "C" void createprocess_tail();

namespace pantheon::arm
{

typedef struct CpuContext
{
	UINT64 x19;
	UINT64 x20;
	UINT64 x21;
	UINT64 x22;
	UINT64 x23;
	UINT64 x24;
	UINT64 x25;
	UINT64 x26;
	UINT64 x27;
	UINT64 x28;
	UINT64 FP;
	UINT64 SP;
	UINT64 PC;

	VOID Wipe()
	{
		x19 = 0;
		x20 = 0;
		x21 = 0;
		x22 = 0;
		x23 = 0;
		x24 = 0;
		x25 = 0;
		x26 = 0;
		x27 = 0;
		x28 = 0;
		FP = 0;
		SP = 0;
		PC = 0;		
	}

	UINT64 &operator[](UINT64 Index)
	{
		if (Index < 19 || Index > 31)
		{
			return x19;
		}

		/* Use some pointer arithmetic to index x19-x31. */
		return *((&x19) + Index);
	}

	[[nodiscard]] 
	UINT64 RegCount() const
	{
		return 32;
	}

	[[nodiscard]] 
	UINT64 FRegCount() const
	{
		return 32;
	}

	VOID SetPC(UINT64 Val)
	{
		this->PC = Val;
	}

	VOID SetSP(UINT64 Val)
	{
		this->SP = Val;
	}

	VOID SetArg1(UINT64 Val)
	{
		this->x20 = Val;
	}

	VOID SetInitContext(UINT64 Fn, UINT64 ArgAddr, UINT64 SP)
	{
		UINT64 TailAddr = (UINT64)(createprocess_tail);
		this->x19 = Fn;
		this->x20 = ArgAddr;
		this->SetSP(SP);
		this->SetPC(TailAddr);
	}

	VOID SetInitUserContext(UINT64 StackAddress, UINT64 StartAddress)
	{
		this->x20 = StartAddress;
		this->x21 = StackAddress;
	}

	CpuContext &operator=(const CpuContext &Other)
	{
		if (this == &Other)
		{
			return *this;
		}

		this->x19 = Other.x19;
		this->x20 = Other.x20;
		this->x21 = Other.x21;
		this->x22 = Other.x22;
		this->x23 = Other.x23;
		this->x24 = Other.x24;
		this->x25 = Other.x25;
		this->x26 = Other.x26;
		this->x27 = Other.x27;
		this->x28 = Other.x28;
		this->FP = Other.FP;
		this->SP = Other.SP;
		this->PC = Other.PC;
		return *this;
	}

}CpuContext;


}

#define CpuIRegOffset offsetof(pantheon::CpuContext, x19)

#endif