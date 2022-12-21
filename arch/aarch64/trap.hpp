#include <kern_datatypes.hpp>


#ifndef _PANTHEON_ARM_TRAP_HPP_
#define _PANTHEON_ARM_TRAP_HPP_

namespace pantheon::arm
{

typedef enum PSTATEMode : UINT64
{
	PSTATE_MODE_USER = 0x00,
	PSTATE_MODE_KERN = 0x04,
	PSTATE_MODE_HYPER = 0x08,
	PSTATE_MODE_FIRM = 0x0C,
}PSTATEMode;

typedef struct TrapFrame
{
	UINT64 Regs[31];
	UINT64 PC;
	PSTATEMode PSTATE;
	UINT64 ESR;
	UINT64 SP;
	UINT64 UNUSED;

	VOID Wipe()
	{
		for (UINT64 &Reg : Regs)
		{
			Reg = 0;
		}
		PC = 0;
		PSTATE = PSTATE_MODE_USER;
		SP = 0;
	}

	VOID SetUser()
	{
		this->PSTATE = PSTATE_MODE_USER;
	}

	VOID SetKern()
	{
		this->PSTATE = PSTATE_MODE_KERN;
	}

	template<typename T>
	T &GetRawArgument(UINT8 Index)
	{
		return (T&)this->Regs[Index];
	}

	UINT64 &GetIntArgument(UINT8 Index)
	{
		return this->Regs[Index];
	}
}TrapFrame;

static_assert(sizeof(TrapFrame) == (8ULL *36ULL));

}


#endif