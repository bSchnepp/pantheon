#include <kern_macro.hpp>
#include <kern_string.hpp>
#include <kern_runtime.hpp>
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
		OBJECT_SELF_ASSERT();
		this->ObjLock.Acquire();
	}

	void Unlock()
	{
		OBJECT_SELF_ASSERT();
		this->ObjLock.Release();
	}

	[[nodiscard]]
	BOOL IsLocked() const
	{
		OBJECT_SELF_ASSERT();
		return this->ObjLock.IsLocked();
	}

private:
	pantheon::Spinlock ObjLock;
};

class ScopedLock
{
public:
	ScopedLock(Lockable *Lk) : Lock(Lk)
	{
		Lock->Lock();
	}

	~ScopedLock()
	{
		Lock->Unlock();
	}
	
private:
	Lockable *Lock;
};

}

#endif