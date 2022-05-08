#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_server_port.hpp>
#include <System/IPC/kern_server_connection.hpp>

#include <System/Proc/kern_sched.hpp>

void pantheon::ipc::ServerConnection::ReplyAndRecv(const UINT32 *Content)
{
	OBJECT_SELF_ASSERT();
	PANTHEON_UNUSED(Content);
	/* NYI */
}

pantheon::Result pantheon::ipc::ServerConnection::RequestHandler(pantheon::Thread *RqThread)
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedGlobalSchedulerLock SchedLock;

	/* Were we already closed? */
	if (this->Owner->IsServerClosed())
	{
		return pantheon::Result::SYS_CONN_CLOSED;
	}

	/* TODO: Sleep the client thread */

	this->WaitingThreads.PushBack(RqThread);
	/* NYI */
	return pantheon::Result::SYS_OK;
}

void pantheon::ipc::ServerConnection::ClientClosedHandler()
{
	OBJECT_SELF_ASSERT();
	this->Cleanup();
}

void pantheon::ipc::ServerConnection::Close()
{
	OBJECT_SELF_ASSERT();
	this->Cleanup();
	this->Owner->CloseServerHandler();
}

void pantheon::ipc::ServerConnection::Cleanup()
{
	/* Mark all threads attempting to push a packet through that the connection is closed. */
	pantheon::ScopedGlobalSchedulerLock SchedLock;
	/* NYI */	
}