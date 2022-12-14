#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/IPC/kern_connection.hpp>

void pantheon::ipc::Connection::CloseServerHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ipc::Connection::State CurrentState = this->CurState.Load();
	if (CurrentState != pantheon::ipc::Connection::State::CLOSED_CLIENT)
	{
		this->CurState = pantheon::ipc::Connection::State::CLOSED_SERVER;
		/* TODO: Fire server closed event for client */
	}
	else
	{
		/* Close it directly */
	}
}

void pantheon::ipc::Connection::CloseClientHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ipc::Connection::State CurrentState = this->CurState.Load();
	if (CurrentState != pantheon::ipc::Connection::State::CLOSED_SERVER)
	{
		this->CurState = pantheon::ipc::Connection::State::CLOSED_CLIENT;
		/* TODO: Fire client closed event for client */
	}
	else
	{
		/* Close it directly */
	}
}

void pantheon::ipc::Connection::Initialize(ClientPort *Client, ServerPort *Server)
{
	OBJECT_SELF_ASSERT();

	this->SrvConn.Initialize(this);
	this->CliConn.Initialize(this);

	this->CliPort = Client;
	this->SrvPort = Server;

	this->CurState = pantheon::ipc::Connection::State::OPEN;
}

void pantheon::ipc::Connection::DestroyObject()
{
	pantheon::ipc::Connection::Destroy(this);
}