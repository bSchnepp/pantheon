#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/IPC/kern_connection.hpp>

void pantheon::ipc::Connection::CloseServer()
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

void pantheon::ipc::Connection::CloseClient()
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