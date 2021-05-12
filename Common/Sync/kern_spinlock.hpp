#include <kern_datatypes.hpp>

#ifndef _KERN_MUTEX_HPP_
#define _KERN_MUTEX_HPP_

namespace pantheon
{

class Spinlock
{
public:
	Spinlock();
	~Spinlock();

	void Acquire();
	void Release();

private:
	BOOL Locked; 
};

}

#endif