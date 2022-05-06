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

void pantheon::ipc::ServerConnection::RequestHandler(pantheon::Thread *RqThread)
{
	OBJECT_SELF_ASSERT();
	PANTHEON_UNUSED(RqThread);
	/* NYI */
}

void pantheon::ipc::ServerConnection::ClientClosedHandler()
{
	OBJECT_SELF_ASSERT();
	this->Cleanup();
	/* NYI */
}

void pantheon::ipc::ServerConnection::Close()
{
	OBJECT_SELF_ASSERT();
	this->Cleanup();
	this->Owner->CloseServerHandler();
	/* NYI */
}

void pantheon::ipc::ServerConnection::Cleanup()
{
	/* Mark all threads attempting to push a packet through that the connection is closed. */
	pantheon::ScopedGlobalSchedulerLock SchedLock;
	/* NYI */	
}