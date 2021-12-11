#include <kern_runtime.hpp>
#include <System/Handle/kern_handle.hpp>

pantheon::Handle::Handle()
{
	this->Type = pantheon::HANDLE_TYPE_INVALID;
	this->Content.ReadEvent = nullptr;
}

pantheon::Handle::Handle(pantheon::ipc::ReadableEvent *Evt)
{
	this->Type = pantheon::HANDLE_TYPE_READ_SIGNAL;
	this->Content.ReadEvent = Evt;
}

pantheon::Handle::Handle(pantheon::ipc::WritableEvent *Evt)
{
	this->Type = pantheon::HANDLE_TYPE_WRITE_SIGNAL;
	this->Content.WriteEvent = Evt;
}

pantheon::Handle::~Handle()
{

}

pantheon::HandleContent &pantheon::Handle::GetContent()
{
	OBJECT_SELF_ASSERT(this);
	return this->Content;
}

pantheon::HandleType pantheon::Handle::GetType()
{
	OBJECT_SELF_ASSERT(this);
	return this->Type;
}