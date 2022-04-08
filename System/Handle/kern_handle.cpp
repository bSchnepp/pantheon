#include <kern_runtime.hpp>
#include <System/Handle/kern_handle.hpp>

#include <System/IPC/kern_event.hpp>
#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_client_port.hpp>
#include <System/IPC/kern_client_connection.hpp>
#include <System/IPC/kern_connection.hpp>

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

pantheon::Handle::Handle(pantheon::ipc::Port *Port)
{
	this->Content.Port = Port;
}

pantheon::Handle::Handle(pantheon::ipc::ClientPort *ClientPort)
{
	this->Content.ClientPort = ClientPort;
}

pantheon::Handle::Handle(pantheon::ipc::Connection *Connection)
{
	this->Content.Connection = Connection;
}

pantheon::Handle::Handle(pantheon::ipc::ClientConnection *ClientConnection)
{
	this->Content.ClientConnection = ClientConnection;
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