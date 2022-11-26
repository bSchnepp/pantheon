#include <kern_result.hpp>
#include <kern_datatypes.hpp>

#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_client_connection.hpp>

#ifndef _KERN_PORT_HPP_
#define _KERN_PORT_HPP_

namespace pantheon::ipc
{

class ClientPort;
class ServerPort;

class ServerConnection;

static const UINT32 PortNameLength = 8;

typedef union PortName
{
	UINT64 AsNumber;
	CHAR AsChars[pantheon::ipc::PortNameLength];
}PortName;

class Port : public pantheon::Allocatable<Port, 128>, public Lockable
{
public:
	Port();
	~Port() override;

	static void Setup();
	static Port *GetRegistered(const PortName &Name);

	void Initialize(PortName Name, INT64 MaxConnections);
	void CloseServerHandler();
	void CloseClientHandler();

	/* TODO: Implement ServerBind(), ClientBind(), etc. */

	[[nodiscard]] BOOL IsServerClosed() const { return this->CurrentState == State::CLOSED_SERVER; }
	[[nodiscard]] BOOL IsClientClosed() const { return this->CurrentState == State::CLOSED_CLIENT; }

	pantheon::ipc::ClientPort *GetClientPort() { return this->Client; }
	pantheon::ipc::ServerPort *GetServerPort() { return this->Server; }

	Result Enqueue(pantheon::ipc::ServerConnection *Conn);
	[[nodiscard]] PortName GetName() const { return this->Name; }

	pantheon::ipc::ClientConnection *CreateConnection();

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
	PortName Name;
	State CurrentState;

	ClientPort *Client;
	ServerPort *Server;
};


static_assert(sizeof(PortName) == pantheon::ipc::PortNameLength);
}

#endif