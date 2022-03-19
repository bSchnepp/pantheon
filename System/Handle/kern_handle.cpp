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

pantheon::Handle::Handle(pantheon::Process *Proc)
{
	this->Type = pantheon::HANDLE_TYPE_PROCESS;
	this->Content.Process = Proc;
}

pantheon::Handle::Handle(pantheon::Thread *Thr)
{
	this->Type = pantheon::HANDLE_TYPE_THREAD;
	this->Content.Thread = Thr;
}

pantheon::Handle::~Handle()
{

}

pantheon::HandleContent &pantheon::Handle::GetContent()
{
	OBJECT_SELF_ASSERT();
	return this->Content;
}

pantheon::HandleType pantheon::Handle::GetType()
{
	OBJECT_SELF_ASSERT();
	return this->Type;
}