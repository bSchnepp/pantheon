#include <kern_datatypes.hpp>
#include <Common/kern_object.hpp>

void pantheon::Object::Open()
{
	this->RefCountUp();
}

void pantheon::Object::Close()
{
	this->RefCountDown();
	if (this->RefCount == 0)
	{
		this->DestroyObject();
	}
}