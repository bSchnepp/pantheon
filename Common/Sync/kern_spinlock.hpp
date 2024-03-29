#include <kern_datatypes.hpp>

#ifndef _KERN_MUTEX_HPP_
#define _KERN_MUTEX_HPP_

namespace pantheon
{

class Spinlock
{
public:
	Spinlock();
	Spinlock(const char *Name);
	~Spinlock();

	void Acquire();
	void Release();

	[[nodiscard]] UINT8 Holder() const;
	[[nodiscard]] BOOL IsLocked() const;

	void SetDebugName(const char *Name);
	const char *GetDebugName();

private:
	[[nodiscard]] BOOL IsHolding() const;
	const char *DebugName;
	UINT16 CoreNo;
	BOOL Locked; 
};

}

#endif