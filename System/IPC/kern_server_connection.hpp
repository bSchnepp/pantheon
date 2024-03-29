#include <kern_result.hpp>
#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <Common/Structures/kern_linkedlist.hpp>

#ifndef _KERN_SERVER_CONNECTION_HPP_
#define _KERN_SERVER_CONNECTION_HPP_

namespace pantheon::ipc
{

class Connection;

class ServerConnection : public pantheon::Object<ServerConnection>
{
public:
	explicit ServerConnection() = default;

	void Initialize(Connection *Owner) { this->Owner = Owner; }
	[[nodiscard]] Connection *GetOwner() const { return this->Owner; }

	void ReplyAndRecv(const UINT32 *Content);
	pantheon::Result RequestHandler(pantheon::Thread *RqThread);
	void ClientClosedHandler();

	void Close();

private:
	Connection *Owner;
	pantheon::LinkedList<pantheon::Thread> WaitingThreads;

private:
	void Cleanup();
};

}

#endif