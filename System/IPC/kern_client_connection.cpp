#include "kern.h"

#include <kern_result.hpp>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_client_port.hpp>
#include <System/IPC/kern_client_connection.hpp>


void pantheon::ipc::ClientConnection::Initialize(pantheon::ipc::Connection *Owner)
{
	OBJECT_SELF_ASSERT();
	this->Owner = Owner;
}

pantheon::Result pantheon::ipc::ClientConnection::Send(pantheon::Thread::ThreadLocalRegion *Region)
{
	OBJECT_SELF_ASSERT();
	PANTHEON_UNUSED(Region);

	/* NYI */
	return pantheon::Result::SYS_OK;
}

pantheon::Result pantheon::ipc::ClientConnection::Recieve(pantheon::Thread::ThreadLocalRegion *Region)
{
	OBJECT_SELF_ASSERT();
	PANTHEON_UNUSED(Region);
	/* NYI */
	return pantheon::Result::SYS_OK;
}

void pantheon::ipc::ClientConnection::ServerClosedHandler()
{
	OBJECT_SELF_ASSERT();
	/* NYI */
}