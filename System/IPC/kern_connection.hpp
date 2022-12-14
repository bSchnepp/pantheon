#include <kern_object.hpp>
#include <kern_datatypes.hpp>

#include <System/Proc/kern_proc.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <System/IPC/kern_server_connection.hpp>
#include <System/IPC/kern_client_connection.hpp>

#include <System/IPC/kern_server_port.hpp>
#include <System/IPC/kern_client_port.hpp>

#ifndef _KERN_CONNECTION_HPP_
#define _KERN_CONNECTION_HPP_

namespace pantheon::ipc
{

class Port;
class ServerConnection;
class ClientConnection;

class Connection : public Allocatable<Connection, 1024>, public pantheon::Object
{
public:
	explicit Connection() = default;

	void Initialize(ClientPort *Client, ServerPort *Server);

	FORCE_INLINE BOOL IsServerClosed()
	{
		return this->CurState == State::CLOSED_SERVER;
	}

	FORCE_INLINE BOOL IsClientClosed()
	{
		return this->CurState == State::CLOSED_CLIENT;
	}

	FORCE_INLINE ServerConnection *GetServerConnection()
	{
		return &this->SrvConn;
	}

	FORCE_INLINE ClientConnection *GetClientConnection()
	{
		return &this->CliConn;
	}

	void CloseServerHandler();
	void CloseClientHandler();

	void DestroyObject() override;



private:
	enum class State : UINT8
	{
		UNUSED,
		OPEN,
		CLOSED_CLIENT,
		CLOSED_SERVER,
		CLOSED,
	};

private:
	pantheon::ipc::ServerConnection SrvConn;
	pantheon::ipc::ClientConnection CliConn;

	pantheon::Atomic<State> CurState;

	pantheon::ipc::ServerPort *SrvPort;
	pantheon::ipc::ClientPort *CliPort;

};

}

#endif