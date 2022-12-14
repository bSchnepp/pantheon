#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_client_port.hpp>
#include <System/IPC/kern_client_connection.hpp>


void pantheon::ipc::ClientConnection::Initialize(pantheon::ipc::Connection *Owner)
{
	this->Owner = Owner;
}

void pantheon::ipc::ClientConnection::Send(const UINT32 *Content)
{
	PANTHEON_UNUSED(Content);
	/* NYI */
}

void pantheon::ipc::ClientConnection::Recieve(UINT32 *Content, UINT64 Size)
{
	ClearBuffer((CHAR*)Content, sizeof(UINT32) * Size);
	/* NYI */
}

void pantheon::ipc::ClientConnection::ServerClosedHandler()
{
	/* NYI */
}

void pantheon::ipc::ClientConnection::DestroyObject()
{
}