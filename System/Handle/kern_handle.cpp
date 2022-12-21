#include <kern_runtime.hpp>
#include <System/Handle/kern_handle.hpp>

#include <System/IPC/kern_event.hpp>
#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_client_port.hpp>
#include <System/IPC/kern_client_connection.hpp>
#include <System/IPC/kern_server_connection.hpp>

pantheon::Handle::Handle()
{
	this->Type = pantheon::HANDLE_TYPE_INVALID;
	this->Content.ReadEvent = nullptr;
}

pantheon::Handle::Handle(pantheon::ipc::ReadableEvent *Evt)
{
	this->Type = pantheon::HANDLE_TYPE_READ_SIGNAL;
	this->Content.ReadEvent = Evt;
	Evt->Open();
}

pantheon::Handle::Handle(pantheon::ipc::WritableEvent *Evt)
{
	this->Type = pantheon::HANDLE_TYPE_WRITE_SIGNAL;
	this->Content.WriteEvent = Evt;
	Evt->Open();
}

pantheon::Handle::Handle(pantheon::Process *Proc)
{
	this->Type = pantheon::HANDLE_TYPE_PROCESS;
	this->Content.Process = Proc;
	Proc->Open();
}

pantheon::Handle::Handle(pantheon::Thread *Thr)
{
	this->Type = pantheon::HANDLE_TYPE_THREAD;
	this->Content.Thread = Thr;
	Thr->Open();
}

pantheon::Handle::Handle(pantheon::ipc::ServerPort *Port)
{
	this->Type = pantheon::HANDLE_TYPE_SERVER_PORT;
	this->Content.ServerPort = Port;
	Port->Open();
}

pantheon::Handle::Handle(pantheon::ipc::ClientPort *ClientPort)
{
	this->Type = pantheon::HANDLE_TYPE_CLIENT_PORT;
	this->Content.ClientPort = ClientPort;
	ClientPort->Open();
}

pantheon::Handle::Handle(pantheon::ipc::ServerConnection *Connection)
{
	this->Type = pantheon::HANDLE_TYPE_SERVER_CONNECTION;
	this->Content.Connection = Connection;
	Connection->Open();
}

pantheon::Handle::Handle(pantheon::ipc::ClientConnection *ClientConnection)
{
	this->Type = pantheon::HANDLE_TYPE_CLIENT_CONNECTION;
	this->Content.ClientConnection = ClientConnection;
	ClientConnection->Open();
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

void pantheon::Handle::Close()
{
	OBJECT_SELF_ASSERT();
	if (this->Type == HANDLE_TYPE_INVALID)
	{
		return;
	}

	switch (this->Type)
	{
		case HANDLE_TYPE_READ_SIGNAL:
		{
			this->Content.ReadEvent->Close();
			break;
		}

		case HANDLE_TYPE_WRITE_SIGNAL:
		{
			this->Content.WriteEvent->Close();
			break;
		}

		case HANDLE_TYPE_PROCESS:
		{
			this->Content.Process->Close();
			break;
		}

		case HANDLE_TYPE_THREAD:
		{
			this->Content.Thread->Close();
			break;
		}

		case HANDLE_TYPE_SERVER_PORT:
		{
			this->Content.ServerPort->Close();
			break;
		}

		case HANDLE_TYPE_CLIENT_PORT:
		{
			this->Content.ClientPort->Close();
			break;
		}

		case HANDLE_TYPE_SERVER_CONNECTION:
		{
			pantheon::ipc::ServerConnection *SrvConn = this->Content.Connection;
			pantheon::ipc::Connection *Conn = SrvConn->GetOwner();
			Conn->CloseServerHandler();
			if (Conn->IsClientClosed() && Conn->IsServerClosed())
			{
				Conn->Close();
			}
			SrvConn->Close();
			break;
		}

		case HANDLE_TYPE_CLIENT_CONNECTION:
		{
			pantheon::ipc::ClientConnection *CliConn = this->Content.ClientConnection;
			pantheon::ipc::Connection *Conn = CliConn->GetOwner();
			Conn->CloseClientHandler();
			if (Conn->IsClientClosed() && Conn->IsServerClosed())
			{
				Conn->Close();
			}
			CliConn->Close();
			break;
		}

		default:
		{
			return;
		}
	}
	*this = pantheon::Handle();
}