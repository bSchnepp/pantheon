#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_SERVER_CONNECTION_HPP_
#define _KERN_SERVER_CONNECTION_HPP_

namespace pantheon::ipc
{

class Connection;

class ServerConnection : public Allocatable<ServerConnection, 1024>
{
public:
	explicit ServerConnection() = default;

	void Initialize(Connection *Owner) { this->Owner = Owner; }
	[[nodiscard]] Connection *GetOwner() const { return this->Owner; }

	void ReplyAndRecv(const UINT32 *Content);
	void RequestHandler(pantheon::Thread *RqThread);
	void ClientClosedHandler();

	void Close();

private:
	Connection *Owner;

private:
	void Cleanup();
};

}

#endif