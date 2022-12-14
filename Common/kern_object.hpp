#include <kern_datatypes.hpp>

#ifndef _KERN_OBJECT_HPP_
#define _KERN_OBJECT_HPP_

namespace pantheon
{

class Object
{
public:
	Object() = default;
	virtual ~Object() = default;

	virtual void Open();
	virtual void Close();

	virtual void DestroyObject() = 0;

protected:
	inline UINT64 RefCountUp() { return ++this->RefCount; }
	inline UINT64 RefCountDown() { return --this->RefCount; }

private:
	UINT64 RefCount;
};

}

#endif