#include <kern_datatypes.hpp>

#ifndef _PANTHEON_ARM_THREAD_HPP_
#define _PANTHEON_ARM_THREAD_HPP_

namespace pantheon::arm
{

typedef struct CpuContext
{
	UINT64 x0;
	UINT64 x1;
	UINT64 x2;
	UINT64 x3;
	UINT64 x4;
	UINT64 x5;
	UINT64 x6;
	UINT64 x7;
	UINT64 x8;
	UINT64 x9;
	UINT64 x10;
	UINT64 x11;
	UINT64 x12;
	UINT64 x13;
	UINT64 x14;
	UINT64 x15;
	UINT64 x16;
	UINT64 x17;
	UINT64 x18;
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
	UINT64 x29;
	UINT64 x30;
	UINT64 x31;	
}CpuContext;

}


#endif