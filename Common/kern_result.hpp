#include <kern_datatypes.hpp>

#ifndef _KERN_RESULT_HPP_
#define _KERN_RESULT_HPP_

namespace pantheon
{

enum class Result : INT32
{
	SYS_FAIL = -1,
	SYS_OK = 0,
};

}


#endif