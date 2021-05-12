

#ifndef _KERN_MUTEX_HPP_
#define _KERN_MUTEX_HPP_

namespace pantheon
{

class Mutex
{
public:
	Mutex();
	~Mutex();

	void Acquire();
	void Release();

private:
	bool Locked; 
};

}

#endif