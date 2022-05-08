#include <kern_datatypes.hpp>

#ifndef _KERN_RESULT_HPP_
#define _KERN_RESULT_HPP_

namespace pantheon
{

enum ResultClass
{
	CLASS_NONE = (0ULL << 24),
	CLASS_KERN = (1ULL << 24),
};

enum class Result : INT32
{
	SYS_OK = 0 | CLASS_NONE,

	SYS_FAIL = 1 | CLASS_KERN,
	SYS_OOM = 2 | CLASS_KERN,

	KERN_CONN_CLOSED = 3 | CLASS_KERN,
	KERN_PORT_CLOSED = 4 | CLASS_KERN,	
};

}


#endif