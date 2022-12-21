#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_client_port.hpp>

void pantheon::ipc::ClientPort::Initialize(pantheon::ipc::Port *Owner, INT64 MaxConnections)
{
	OBJECT_SELF_ASSERT();
	this->Owner = Owner;
	this->MaxConnectionCount = MaxConnections;
}

[[nodiscard]]
BOOL pantheon::ipc::ClientPort::IsServerClosed() const
{
	OBJECT_SELF_ASSERT();
	return this->Owner->IsServerClosed();
}