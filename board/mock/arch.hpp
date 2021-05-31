#include <kern.h>
#include <kern_datatypes.hpp>

#ifndef MOCK_ARCH_HPP_
#define MOCK_ARCH_HPP_

namespace pantheon
{

typedef struct CpuContext
{
	BOOL Nothing;
}CpuContext;

VOID RearmSystemTimer(UINT64 Freq);

}

#endif