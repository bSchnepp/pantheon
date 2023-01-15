#include <kern_result.hpp>
#include <kern_object.hpp>
#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <System/Proc/kern_thread.hpp>

#ifndef _KERN_CLIENT_CONNECTION_HPP_
#define _KERN_CLIENT_CONNECTION_HPP_

namespace pantheon::ipc
{

class Connection;

class ClientConnection : public pantheon::Object<ClientConnection>
{
public:
	explicit ClientConnection() = default;

	void Initialize(Connection *Owner);

	[[nodiscard]] Connection *GetOwner() const { return this->Owner; }

	pantheon::Result Send(pantheon::Thread::ThreadLocalRegion *Region);
	pantheon::Result Recieve(pantheon::Thread::ThreadLocalRegion *Region);

	void ServerClosedHandler();

private:
	Connection *Owner;
};

}

#endif