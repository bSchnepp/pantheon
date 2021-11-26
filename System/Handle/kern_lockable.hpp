#include <kern_macro.hpp>
#include <kern_string.hpp>
#include <Sync/kern_spinlock.hpp>

#ifndef _KERN_LOCKABLE_HPP_
#define _KERN_LOCKABLE_HPP_

namespace pantheon
{

class Lockable
{
public:
	Lockable() : Lockable("unknown")
	{
	}

	Lockable(const char *Name)
	{
		this->ObjLock = pantheon::Spinlock(Name);
	}

	virtual ~Lockable()
	{
		if (this->ObjLock.IsLocked())
		{
			this->ObjLock.Release();
		}
	}

	void Lock()
	{
		this->ObjLock.Acquire();
	}

	void Unlock()
	{
		this->ObjLock.Release();
	}

	BOOL IsLocked()
	{
		return this->ObjLock.IsLocked();
	}

private:
	pantheon::Spinlock ObjLock;
};

}

#endif