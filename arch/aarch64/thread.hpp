#include <kern_datatypes.hpp>

#ifndef _PANTHEON_ARM_THREAD_HPP_
#define _PANTHEON_ARM_THREAD_HPP_

namespace pantheon::arm
{

typedef struct CpuContext
{
	UINT64 Regs[32];

	VOID Wipe()
	{
		for (UINT64 &Item : this->Regs)
		{
			Item = 0;
		}
	}

	UINT64 &operator[](UINT64 Index)
	{
		return this->Regs[Index];
	}
	
}CpuContext;

}


#endif